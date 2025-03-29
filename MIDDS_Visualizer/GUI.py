import dash
from dash import dcc, html, no_update, ClientsideFunction, clientside_callback
from dash.exceptions import PreventUpdate
from dash.dependencies import Input, Output, State
import plotly.graph_objs as go
from ProgramConfiguration import ProgramConfiguration
from GUI2BackendEvents import GUI2BackendEvents
import json
from MIDDSChannel import MIDDSChannelOptions
import copy

class GUI:
    def __init__(self, lock, events: GUI2BackendEvents, config):
        self.channelsLock = lock
        self.events = events
        self.config: ProgramConfiguration = config

        self.temporalConfig: ProgramConfiguration = copy.deepcopy(config)
        self.changesToApply: bool = False
        self.suppressChannelOptionsUpdate = False

        self.selectedChannelNumber:   int = 0

        self.app = dash.Dash(__name__, external_stylesheets=["/assets/style.css"])
        self.app.title = "MIDDS"

        # Update interval (ms).
        self.interval = 500  

        # GPIO state tracking
        self.gpio_states = {i: {"last_read": "N/A", "state": "LOW", "history": []} for i in range(1, 15)}

        self.setupLayout()
        self.setupCallbacks()
        self.setupClientCallbacks()

    def run(self, debug=False):
        self.app.run(debug=debug,dev_tools_silence_routes_logging=True)

    def setupLayout(self):
        # Initial figure.
        self.fig = go.Figure()
        self.fig.update_layout(
            xaxis=dict(
                title = 'Time',
                tickformat = '%H:%M:%S',
                autorange = True
            ),
            yaxis=dict(
                title = 'Frequency (Hz)',
                autorange = True
            ),
            
            template='plotly_dark',
            margin=dict(l=20, r=20, t=20, b=20),
            showlegend=True,

            # Transition breaks the autorange function...
            # transition={'duration': 200}
        )

        # App layout
        self.app.layout = html.Div([
            # Header
            html.Header([
                html.Button("☰", id="toggle-sidebar", className="sidebar-btn"),
                html.H2("MIDDS Visualizer", className="title"),

                html.Div([
                    dcc.Input(id="serial-name", className="inputPane", type="text", placeholder="Serial port", 
                            value=self.config['ProgramConfig']['SERIAL_PORT']),
                    html.Button("Connect", id="connect-btn", className="connect-btn", n_clicks=0),
                    html.Button("⏺ Record", id="record-btn", className="record-btn", n_clicks=0)
                ], className="header-right-div")
            ], className="header"),
            
            # Sidebar
            html.Div([
                html.Div([
                    html.Label("Edit channel", className="section-title"),
                    dcc.Dropdown(
                        id="gpio-channel", 
                        options=[{
                                    "label": f"Channel {i}", 
                                    "value": i
                                } for i in range(0, int(self.config['ProgramConfig']['CHANNEL_COUNT']))], 
                                value=self.selectedChannelNumber),
                    
                    html.Hr(),

                    html.Label("Name:", className="sidebar-label"),
                    dcc.Input(id="gpio-name", className="inputPane", type="text", placeholder="Enter channel name", 
                            value=self.config.getChannel(self.selectedChannelNumber).name),
                    
                    html.Label("Mode:", className="sidebar-label"),
                    dcc.Dropdown(
                        id="gpio-mode", 
                        options=[
                            {"label": "Disabled",               "value": "DS"},
                            {"label": "Input",                  "value": "IN"}, 
                            {"label": "Output",                 "value": "OU"},
                            {"label": "Monitor Rising edges",   "value": "MR"},
                            {"label": "Monitor falling edges",  "value": "MF"},
                            {"label": "Monitor both edges",     "value": "MB"},
                        ], 
                        value=self.config.getChannel(self.selectedChannelNumber).mode
                    ),

                    html.Label("Signal type:", className="sidebar-label"),
                    dcc.RadioItems(
                        id="gpio-signal", className="gpio-signal",
                        options = [
                            {"label": "TTL",    "value": "T"},
                            {"label": "LVDS",    "value": "L"},
                        ],
                        value=self.config.getChannel(self.selectedChannelNumber).signal
                    ),

                    html.Label("Channel options:", className="sidebar-label"),
                    html.Div(id="gpio-options"),
                ], className="sidebar-content"),

                html.Div([
                    html.P("You've made changes to the channel's configuration."),
                    html.Div([
                        html.Button("Apply", id="apply-config-btn", className="config-btn", n_clicks=0),
                        html.Button("Discard", id="discard-config-btn", className="config-btn", n_clicks=0)
                    ], className="apply-config-button-div")
                ], id="apply-config", className="apply-config hidden")
            ], id="sidebar", className="sidebar"),
            
            # Main Content
            html.Main([
                # Frequency Graph
                html.Section([
                    html.Label("Frequencies", className="section-title"),
                    dcc.Graph(id='freq-graph', figure=self.fig),
                    dcc.Checklist(id='freq-filters', options=[], inline=True)
                ], className="freq-section"),
                
                html.Hr(),

                # Inputs Row
                html.Section([
                    html.Label("Inputs", className="section-title"),
                    html.Section(id="input-channels", className="input-channels"),
                ],className="inputs-row"),
                
                html.Hr(),

                # Outputs Row
                html.Section([
                    html.Label("Outputs", className="section-title"),
                    html.Section(id="output-channels", className="output-channels"),
                ],className="outputs-row"),
            ], id="main-content", className="main-content"),
            
            # Error Notification
            html.Div([
                html.Button("✖", className="close-error-div"),
                html.P(id="error-title",    className="error-title"),
                html.P(id="error-date",     className="error-date"),
                html.P(id="error-content",  className="error-content")
            ], id="error-message-div", className="error-message-div initialHidden"),
            
            # Footer
            html.Footer([
                html.P([
                    "Made by ",
                    html.A("@dabecart", href="https://www.instagram.com/dabecart", target="_blank")
                ])
            ], className="footer"),
            
            # Interval Component for Live Updates
            dcc.Interval(id='interval-component', interval=self.interval, n_intervals=0),
            dcc.Store(id="clientServerStore", data='{}')
        ], className="parentDiv")

    def setupCallbacks(self):
        # Callback for toggling sidebar visibility.
        @self.app.callback(
            Output("sidebar", "className"),
            Output("main-content", "className"),
            Input("toggle-sidebar", "n_clicks"),
            State("sidebar", "className")
        )
        def toggle_sidebar(n_clicks, current_class):
            if n_clicks and "hidden" not in current_class:
                return "sidebar hidden", "main-content"
            return "sidebar", "main-content sidebar-open"

        # Callback for connecting/disconnecting from the device.
        @self.app.callback(
            Input("record-btn", "n_clicks"),
            prevent_initial_call=True,
        )
        def toggleRecording(n_clicks):
            if not self.events.deviceConnected:
                self.events.raiseError("Cannot record",
                                       "Connect to MIDDS first by entering the serial port and clicking the 'Connect' button.")
                return
                
            if self.events.recording:
                # The computer is recording and by clicking, the user is requesting to stop.
                self.events.stopRecording.set()
            else:
                self.events.startRecording.set()
        
        @self.app.callback(
            Output("record-btn", "children"),
            Output("record-btn", "className"),
            Input("interval-component", "n_intervals"),
            prevent_initial_call=True
        )
        def updateRecordButton(n):
            if self.events.startRecording.is_set() or self.events.stopRecording.is_set():
                return dcc.Loading(id="loading-display", display="show"), "record-btn"

            if self.events.recording and self.events.deviceConnected:
                return "⬜ Recording", "record-btn connected"
            else:
                return "⏺ Record", "record-btn"

        # Callback for connecting/disconnecting from the device.
        @self.app.callback(
            Input("connect-btn", "n_clicks"),
            prevent_initial_call=True
        )
        def toggleConnection(n_clicks):
            if self.events.deviceConnected:
                # The device is connected and by clicking, the user is requesting to disconnect.
                self.events.closeSerialPort.set()
                if self.events.recording:
                    # If the device is being recorded and a disconnection is requested, stop the 
                    # recording too.
                    self.events.stopRecording.set()
            else:
                self.events.openSerialPort.set()
        
        @self.app.callback(
            Output("connect-btn", "children"),
            Output("connect-btn", "className"),
            Output("connect-btn", "disabled"),
            Input("interval-component", "n_intervals")
        )
        def updateConnectButton(n):
            if self.events.openSerialPort.is_set() or self.events.closeSerialPort.is_set():
                return dcc.Loading(id="loading-display", display="show"), "connect-btn", True

            if self.events.deviceConnected:
                return "Connected", "connect-btn connected", False
            else:
                return "Connect", "connect-btn", False

        # Callback for serial name change.
        @self.app.callback(
            Input("serial-name", "value"),
            prevent_initial_call=True,
        )
        def updateSerialName(serialName):
            self.config['ProgramConfig']['SERIAL_PORT'] = serialName
            self.config.saveConfig()

        # Callback for sidebar changes.
        @self.app.callback(
            Output("gpio-name", "value"),
            Output("gpio-mode", "value"),
            Output("gpio-signal", "value"),
            Output("apply-config", "className", allow_duplicate=True),

            Input("gpio-channel", "value"),
            Input("gpio-name", "value"),
            Input("gpio-mode", "value"),
            Input("gpio-signal", "value"),
            prevent_initial_call=True,
        )
        def updateChannelConfig(channel, name, mode, signal):
            if channel is None or name is None or mode is None or signal is None:
                raise PreventUpdate
            
            returnName = no_update
            returnMode = no_update
            returnSignal = no_update
            applyConfigClassName = no_update

            selChannel = self.temporalConfig.getChannel(channel)
            if selChannel is None:
                raise PreventUpdate
            
            if self.selectedChannelNumber == channel:
                # The selected channel is the same, therefore some data has changed in the GUI.
                if selChannel.name != name or selChannel.mode != mode or selChannel.signal != signal:
                    # Unhide the configuration div by removing the hidden class.
                    applyConfigClassName = "apply-config"
                
                if selChannel.mode != mode:
                    # Remove channel settings from previous modes and add the default ones.
                    selChannel.mode = mode
                    selChannel.filterModeSettings()
                    selChannel.modeSettings = MIDDSChannelOptions.getDefaultChannelOptionsForMode(selChannel.mode)

                selChannel.name = name
                selChannel.signal = signal
            else:
                # The selected channel has changed.
                self.selectedChannelNumber = channel
                # Update the GUI with the new channel information.
                returnName = selChannel.name
                returnMode = selChannel.mode
                returnSignal = selChannel.signal

            # apply-config without the hidden.
            return returnName, returnMode, returnSignal, applyConfigClassName

        # Callback for applying the new configuration.
        @self.app.callback(
            Output("apply-config", "className", allow_duplicate=True),
            Input("apply-config-btn", "n_clicks"),
            prevent_initial_call=True,
        )
        def applyNewConfiguration(n_clicks):
            if not self.events.deviceConnected:
                self.events.raiseError("Cannot apply configuration",
                                     "Connect to MIDDS first by entering the serial port and clicking the 'Connect' button.")
                raise PreventUpdate

            self.channelsLock.acquire()
            
            self.config.copyFrom(self.temporalConfig)

            self.config.saveConfig()
            self.events.applyConfiguration.set()
            self.channelsLock.release()
            return "apply-config hidden"

        # Callback for not applying the new configuration.
        @self.app.callback(
            Output("apply-config", "className", allow_duplicate=True),
            Input("discard-config-btn", "n_clicks"),
            prevent_initial_call=True,
        )
        def discardNewConfiguration(n_clicks):
            self.temporalConfig.copyFrom(self.config)
            # TODO: Update all sidebar configuration fields when configuration is discarded.
            return "apply-config hidden"

        # Callback for GPIO options.
        @self.app.callback(
            Output('gpio-options', 'children'),
            Input('gpio-mode', 'value'),
        )
        def updateGPIOOptions(mode):
            # The update of gpio-options should not trigger the updateAddToFreqGraph callback.
            self.suppressChannelOptionsUpdate = True
            if mode == "IN":
                return html.Div([
                    dcc.Checklist(id="gpio-options-list", 
                                  options=MIDDSChannelOptions.getGUIOptionsForMode("IN"),
                                  value=self.temporalConfig.getChannel(self.selectedChannelNumber).getChannelOptionsForChecklist(),
                                  inline=True)
                ])
            return html.Div()

        @self.app.callback(
            Output('gpio-options-list', 'value'),
            Output("apply-config", "className", allow_duplicate=True),

            State("gpio-channel", "value"),
            Input("gpio-options-list", "value"),
            prevent_initial_call=True,
            suppress_callback_exceptions=True
        )
        def updateAddToFreqGraph(channel, inChecklist):
            if self.suppressChannelOptionsUpdate:
                self.suppressChannelOptionsUpdate = False
                raise PreventUpdate

            selChannel = self.temporalConfig.getChannel(channel)
            if selChannel is None:
                raise PreventUpdate
            
            inCheckListRet = no_update
            applyConfigClassNameRet = no_update

            if self.selectedChannelNumber == channel:
                # Same channel number, the checkboxes must have changed.
                for key in selChannel.modeSettings.keys():
                    # Update checkboxes.
                    if type(selChannel.modeSettings[key]) is bool:
                        newKeyValue: bool = key in inChecklist
                        if newKeyValue != selChannel.modeSettings[key]:
                            applyConfigClassNameRet = "apply-config"
                        
                        selChannel.modeSettings[key] = newKeyValue
            else:
                inCheckListRet = list(selChannel.modeSettings.keys())

            return inCheckListRet, applyConfigClassNameRet

        # Callback to update input/output/frequency widgets
        @self.app.callback(
            Output('freq-graph', 'figure'),
            Output("input-channels", "children"),
            Output("output-channels", "children"),
            
            Input("interval-component", "n_intervals"),
            State('freq-graph', 'figure')
        )
        def updateWidgets(n, f):
            if not self.events.deviceConnected:
                raise PreventUpdate
            
            # Update the figure with the settings of the client side.
            self.fig = go.Figure(f)

            # fig = go.Figure()
            figUpdated: bool = False

            inputWidgets = []
            outputWidgets = []

            self.channelsLock.acquire()
            for ch in self.config.channels:
                title = f"Ch. {ch.number:02}"
                if ch.name != "":
                    title += f": {ch.name}" 

                if ch.mode == "IN":
                    inputWidgets.append(
                        html.Div([
                            html.P(title),
                            html.P(f"Level: {ch.channelLevel}"),
                            html.P(f"Freq:  {ch.frequency}"),
                            html.P(f"Signal type:  {ch.signalType}")
                        ], className="gpio-widget")
                    )
                elif ch.mode == "OU":
                    outputWidgets.append(
                        html.Div([
                            html.P(title),
                            html.P(f"Level: {ch.channelLevel}"),
                            html.Button("ON", id=f"gpio-on-{ch.number}", className="gpio-btn"),
                            html.Button("OFF", id=f"gpio-off-{ch.number}", className="gpio-btn")
                        ], className="gpio-widget")
                    )

                plotInFreqGraph = ch.modeSettings.get("INPlotFreqInGraph", False)
                channelNameInGraph = f'Ch. {ch.number:02}'
                # list of traces with the name "channelNameInGraph".
                foundMatches = list(self.fig.select_traces(selector=dict(name=channelNameInGraph)))

                if not plotInFreqGraph and len(foundMatches) > 0:
                    # Remove the trace from the graph if it's not set to plot.
                    for i, trace in enumerate(self.fig['data']):
                        if 'name' in trace and trace['name'] == channelNameInGraph:
                            # self.fig['data] returns a tuple. Remove the trace from it.
                            dataList = list(self.fig['data'])
                            del dataList[i]
                            self.fig['data'] = tuple(dataList)
                            break
                    
                if plotInFreqGraph and ch.mode == "IN":
                    if len(ch.freqs) <= 0:
                        continue

                    xPoints = tuple(ch.freqsUpdates)
                    yPoints = tuple(ch.freqs)

                    if len(foundMatches) > 0:
                        # If the trace is on the graph, update it.
                        foundMatches[0]['x'] = xPoints
                        foundMatches[0]['y'] = yPoints
                        
                    else:
                        figureColors: list[str] = ['#FF6969',
                                                   '#FFB860', 
                                                   '#FAFF71', 
                                                   '#8DFF76', 
                                                   '#58F1FF', 
                                                   '#5C9AFF', 
                                                   '#836DFF', 
                                                   '#FF7EFF', 
                                                   '#FFFFA9']
                        # If the trace is not on the graph, add it.
                        self.fig.add_trace(go.Scatter(
                            x=xPoints,
                            y=yPoints,
                            mode='markers+lines',
                            marker=dict(size=6, color=figureColors[len(self.fig.data) % len(figureColors)], opacity=0.7),
                            name=channelNameInGraph
                        ))

                    figUpdated = True

            self.channelsLock.release()

            return self.fig if figUpdated else no_update, inputWidgets, outputWidgets

        # Callback to update error messages.
        @self.app.callback(
            Output('error-title',       'children'),
            Output('error-date',        'children'),
            Output('error-content',     'children'),
            Output('clientServerStore', 'data', allow_duplicate=True),

            Input('interval-component', 'n_intervals'),
            State('clientServerStore', 'data'),
            prevent_initial_call=True
        )
        def updateErrors(n, clientServerStore):
            if not self.events.newMIDDSError.is_set() and not self.events.newMIDDSMessage.is_set():
                raise PreventUpdate

            loadedStore = json.loads(clientServerStore)
            if self.events.newMIDDSError.is_set():
                self.events.newMIDDSError.clear()
                # Remove the hidden class from error-message-div and add the color scheme.
                loadedStore['error-message-div'] = 'error-message-div error-scheme'
                return self.events.errTitle, \
                       self.events.errDate.strftime("%m/%d/%Y, %H:%M:%S"), \
                       self.events.errContent, json.dumps(loadedStore)


            if self.events.newMIDDSMessage.is_set():
                self.events.newMIDDSMessage.clear()
                loadedStore['error-message-div'] = 'error-message-div message-scheme'
                return self.events.msgTitle, \
                       self.events.msgDate.strftime("%m/%d/%Y, %H:%M:%S"), \
                       self.events.msgContent, json.dumps(loadedStore)

        @self.app.callback(
            Output('error-message-div', 'className'),
            Input('clientServerStore', 'data'),
            prevent_initial_call=True
        )
        def updateInterfaceFromClient(clientServerStore):
            loadedStore = json.loads(clientServerStore)
            return loadedStore['error-message-div']

    def setupClientCallbacks(self):
        self.app.clientside_callback(
            ClientsideFunction(
                namespace='clientside',
                function_name='closeErrorOnClick'
            ),
            Input("error-message-div", "className"),
            prevent_initial_call=True
        )

        self.app.clientside_callback(
            ClientsideFunction(
                namespace='clientside',
                function_name='updateInterfaceFromClient'
            ),
            Output('clientServerStore', 'data', allow_duplicate=True),
            Input('interval-component', 'n_intervals'),
            Input('clientServerStore', 'data'),
            prevent_initial_call=True
        )
