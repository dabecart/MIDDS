import dash
from dash import dcc, html, no_update, ClientsideFunction, ctx, ALL
from dash.exceptions import PreventUpdate
from dash.dependencies import Input, Output, State
import plotly.graph_objs as go

import json
import copy

from MIDDSChannel import MIDDSChannel, MIDDSChannelOptions
from MIDDSParser import MIDDSParser
from ProgramConfiguration import ProgramConfiguration
from GUI2BackendEvents import GUI2BackendEvents

class GUI:
    def __init__(self, lock, events: GUI2BackendEvents, config):
        self.channelsLock = lock
        self.events = events
        self.config: ProgramConfiguration = config

        self.temporalConfig: ProgramConfiguration = copy.deepcopy(config)
        self.changesToApply: bool = False
        self.suppressChannelOptionsUpdate = False
        self.plotPeriodInsteadOfFreq = False

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
        self.frequencyFig = go.Figure()
        self.frequencyFig.update_layout(
            xaxis=dict(
                # title = 'Time',
                tickformat = '%H:%M:%S',
                autorange = True
            ),
            yaxis=dict(
                title = 'Frequency (Hz)',
                autorange = True
            ),
            
            template='plotly_dark',
            margin=dict(l=10, r=10, t=5, b=20),
            showlegend=True,
            paper_bgcolor='rgba(0,0,0,0)',
            plot_bgcolor='rgba(0,0,0,0)',

            # Transition breaks the autorange function...
            # transition={'duration': 200}
        )

        self.dutyCycleFig = go.Figure()
        self.dutyCycleFig.update_layout(
            xaxis=dict(
                # title = 'Time',
                tickformat = '%H:%M:%S',
                autorange = True
            ),
            yaxis=dict(
                title = 'Duty cycle (%)',
                autorange = True
            ),
            
            template='plotly_dark',
            margin=dict(l=10, r=10, t=5, b=20),
            showlegend=True,
            paper_bgcolor='rgba(0,0,0,0)',
            plot_bgcolor='rgba(0,0,0,0)',
        )

        self.deltaFig = go.Figure()
        self.deltaFig.update_layout(
            xaxis=dict(
                # title = 'Delta (ns)',
                autorange = True
            ),
            yaxis=dict(
                title = 'Frequency',
                autorange = True
            ),
            
            template='plotly_dark',
            margin=dict(l=10, r=10, t=5, b=20),
            showlegend=True,
            paper_bgcolor='rgba(0,0,0,0)',
            plot_bgcolor='rgba(0,0,0,0)',
        )

        # App layout
        self.app.layout = html.Div([
            # Header
            html.Header([
                html.Div([
                    html.Button(
                        html.Img(src="assets/icons/menu.svg", className="header-round-btn-image"),
                    id="toggle-sidebar", className="header-round-btn"),
                    # html.Button(
                    #     html.Img(src="assets/icons/open.svg", className="header-round-btn-image"),
                    # id="open-file", className="header-round-btn"),
                    # html.Button(
                    #     html.Img(src="assets/icons/save.svg", className="header-round-btn-image"),
                    # id="save-file", className="header-round-btn")
                ], className="header-top-left-buttons"),

                html.H2("MIDDS Visualizer", className="title"),

                html.Div([
                    dcc.Input(id="serial-name", className="inputPane", type="text", placeholder="Serial port", 
                            value=self.config['ProgramConfig']['SERIAL_PORT']),
                    html.Button("Connect", id="connect-btn", className="connect-btn"),
                    html.Button("⏺ Record", id="record-btn", className="record-btn")
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
                                value=self.selectedChannelNumber,
                        clearable=False
                    ),
                    
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
                        value=self.config.getChannel(self.selectedChannelNumber).mode,
                        clearable=False
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

                    html.Hr(),

                    html.Label("Channel functions:", className="sidebar-label"),
                    html.Button("Clear channel's data", id="clear-data-btn", className="clear-data-btn"),
                    html.Button("Clear all channels' data", id="clear-all-data-btn", className="clear-data-btn"),
                ], className="sidebar-content"),

                html.Div([
                    html.P("You've made changes to the channel's configuration."),
                    html.Div([
                        html.Button("Discard", id="discard-config-btn", className="config-btn"),
                        html.Button("Apply", id="apply-config-btn", className="config-btn"),
                    ], className="apply-config-button-div")
                ], id="apply-config", className="apply-config hidden")
            ], id="sidebar", className="sidebar"),
            
            # Main Content
            html.Main([
                # Frequency Graph
                html.Section([
                    html.Section([
                        html.Div([
                            html.Label("Periods" if self.plotPeriodInsteadOfFreq else "Frequencies",
                                       id="switch-frequencies-label", className="section-title"),
                            html.Button("Switch to frequencies" if self.plotPeriodInsteadOfFreq else "Switch to periods",
                                         id="switch-frequencies-btn", className="switch-frequencies-btn"),
                        ], className="freq-label-div"),
                        dcc.Graph(id='freq-graph', className='graph', figure=self.frequencyFig)
                    ], className="freq-section"),

                    html.Section([
                        html.Label("Duty Cycles", className="section-title"),
                        dcc.Graph(id='duty-graph', className='graph', figure=self.dutyCycleFig)
                    ], className="duty-section"),

                    html.Section([
                        html.Label("Time deltas", className="section-title"),
                        dcc.Graph(id='deltas-graph', className='graph', figure=self.deltaFig)
                    ], className="deltas-section"),
                ], className="plots-row"),
                
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

                html.Hr(),

                # Monitor Row
                html.Section([
                    html.Label("Monitors", className="section-title"),
                    html.Section(id="monitor-channels", className="monitor-channels"),
                ],className="monitors-row"),

            ], id="main-content", className="main-content"),
            
            # Error Notification
            html.Div([
                html.Button("✖", className="close-error-div"),
                html.P(id="error-title",    className="error-title"),
                html.P(id="error-date",     className="error-date"),
                html.P(id="error-content",  className="error-content")
            ], id="error-message-div", className="error-message-div initialHidden"),
            
            # Settings pane
            html.Div([
                html.P("Settings", id="settings-title", className="settings-title"),
                html.Div(className="settings-div-content", id="settings-div-content"),
                html.Div([
                    html.Button("Discard settings", className="config-btn", id="discard-settings-btn"),
                    html.Button("Apply settings", className="config-btn", id="apply-settings-btn"),
                ], className="apply-settings-div"),
            ], id="settings-div", className="settings-div initialHidden"),

            # Footer
            html.Footer([
                html.Button(
                    html.Img(src="assets/icons/settings.svg", className="header-round-btn-image"),
                id="settings-btn", className="header-round-btn settings-btn"),
                html.P([
                    "Made by ",
                    html.A("@dabecart", href="https://www.instagram.com/dabecart", target="_blank")
                ])
            ], className="footer"),
            
            html.Div(id="dummy", style={"display": "none"}),
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

        # Callback to toggle the recording of the device.
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
            Input("serial-name", "n_submit"),
            State("serial-name", "value"),
            prevent_initial_call=True
        )
        def toggleConnection(n_clicks, n_submits, serialName):
            self.config['ProgramConfig']['SERIAL_PORT'] = serialName
            self.config.saveConfig()

            if ctx.triggered_id == "serial-name" and self.events.deviceConnected:
                # The enter key should only be used to connect, not to disconnect.
                return

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

        @self.app.callback(
            Input("clear-data-btn", "n_clicks"),
            prevent_initial_call=True
        )
        def clearDataPoints(clicks):
            self.config.getChannel(self.selectedChannelNumber).clearValues()

        @self.app.callback(
            Input("clear-all-data-btn", "n_clicks"),
            prevent_initial_call=True
        )
        def clearAllDataPoints(clicks):
            for ch in self.config.channels:
                ch.clearValues()

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
            self.events.applyChannelsConfiguration.set()
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
            optionsList = MIDDSChannelOptions.getGUIOptionsForMode(mode)
            if len(optionsList) > 0:
                return html.Div([
                    dcc.Checklist(id="gpio-options-list", 
                                  options=optionsList,
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

        @self.app.callback(
            Output("switch-frequencies-label", "children"),
            Output("switch-frequencies-btn", "children"),
            Input("switch-frequencies-btn", "n_clicks"),
            prevent_initial_call=True,
        )
        def switchFreqGraphUnits(n_clicks):
            self.plotPeriodInsteadOfFreq = not self.plotPeriodInsteadOfFreq

            return "Periods" if self.plotPeriodInsteadOfFreq else "Frequencies", \
                   "Switch to frequencies" if self.plotPeriodInsteadOfFreq else "Switch to periods"

        # Callback to update input/output/frequency widgets
        @self.app.callback(
            Output('freq-graph', 'figure'),
            Output('duty-graph', 'figure'),
            Output('deltas-graph', 'figure'),
            Output("input-channels", "children"),
            Output("output-channels", "children"),
            Output("monitor-channels", "children"),
            
            Input("interval-component", "n_intervals"),
            State('freq-graph', 'figure'),
            State('duty-graph', 'figure'),
            State('deltas-graph', 'figure')
        )
        def updateWidgets(n, freqGraph, dutyGraph, deltasGraph):
            def updateDataInPlot(ch: MIDDSChannel, figure: go.Figure, xPoints: tuple, yPoints: tuple|None, plotInGraph: bool) -> bool:
                figureColors: tuple[str] = ('#FF6969', 
                                            '#FFB860', 
                                            '#FAFF71', 
                                            '#8DFF76', 
                                            '#58F1FF', 
                                            '#5C9AFF', 
                                            '#836DFF', 
                                            '#FF7EFF', 
                                            '#FFFFA9')

                channelNameInGraph = f'Ch. {ch.number:02}'
                # list of traces with the name "channelNameInGraph".
                foundMatches = list(figure.select_traces(selector=dict(name=channelNameInGraph)))

                if plotInGraph:
                    if len(xPoints) <= 0: return False

                    if len(foundMatches) > 0:
                        # If the trace is on the graph, update it.
                        foundMatches[0]['x'] = xPoints
                        if yPoints is not None:
                            foundMatches[0]['y'] = yPoints
                        
                    else:
                        # If the trace is not on the graph, add it.
                        if yPoints is None:
                            # Add a histogram graph.
                            figure.add_trace(go.Histogram(
                                x=xPoints,
                                nbinsx=50,
                                marker=dict(color=figureColors[len(figure.data) % len(figureColors)]),
                                opacity=0.7,
                                name=channelNameInGraph
                            ))
                        else:
                            # Add an Scatter graph.
                            figure.add_trace(go.Scatter(
                                x=xPoints,
                                y=yPoints,
                                mode='markers+lines',
                                marker=dict(size=4,
                                            color=figureColors[len(figure.data) % len(figureColors)], 
                                            opacity=0.7),
                                name=channelNameInGraph
                            ))
                    return True
                
                elif len(foundMatches) > 0:
                    # Remove the trace from the graph if it's not set to plot.
                    for i, trace in enumerate(figure['data']):
                        if 'name' in trace and trace['name'] == channelNameInGraph:
                            # self.fig['data] returns a tuple. Remove the trace from it.
                            dataList = list(figure['data'])
                            del dataList[i]
                            figure['data'] = tuple(dataList)
                            return True

                return False

            # Update the figures with the settings of the client side.
            self.frequencyFig   = go.Figure(freqGraph)
            self.dutyCycleFig   = go.Figure(dutyGraph)
            self.deltaFig       = go.Figure(deltasGraph)

            retFreqGraph = no_update
            retDutyGraph = no_update
            retDeltaGraph = no_update
            inputWidgets = []
            outputWidgets = []
            monitorWidgets = []

            self.channelsLock.acquire()
            for ch in self.config.channels:
                title = f"Ch. {ch.number:02}"
                if ch.name != "":
                    title += f": {ch.name}" 

                if ch.mode == "IN":
                    inputContent = [
                        html.P(title),
                        html.P(ch.signalType),
                    ]

                    if ch.modeSettings.get("INRequestLevel", False):
                        inputContent.append(
                            html.P(f"Level:      {ch.channelLevel}")
                        )
                    
                    if ch.modeSettings.get("INRequestFrequency", False):
                        inputContent.extend([
                            html.P(f"Frequency:  {ch.frequency} Hz"),
                            html.P(f"Duty cycle: {ch.dutyCycle} %"),
                        ])

                    inputWidgets.append(
                        html.Div(inputContent, className="gpio-widget input-gpio-widget")
                    )

                elif ch.mode == "OU":
                    outputContent = [
                        html.P(title),
                        html.P(ch.signalType),
                        html.P(f"Level:      {ch.channelLevel}"),
                        html.Button("HIGH", 
                                    id={"type": "gpio-set-high", "index": ch.number}, 
                                    className="gpio-btn"),
                        html.Button("LOW",  
                                    id={"type": "gpio-set-low", "index": ch.number}, 
                                    className="gpio-btn"),
                    ]

                    outputWidgets.append(
                        html.Div(outputContent, className="gpio-widget output-gpio-widget")
                    )
                elif ch.mode in ["MR", "MF", "MB"]:
                    monitorContent = [
                        html.P(title),
                        html.P(ch.signalType),
                    ]

                    if ch.modeSettings.get(ch.mode + "CalculateFrequency", False):
                        monitorContent.extend([
                            html.P(f"Frequency:  {ch.frequency} Hz"),
                            html.P(f"Duty cycle: {ch.dutyCycle} %"),
                        ])

                    if ch.mode in ["MR", "MB"]:
                        monitorContent.append(
                            html.P(f"Rising Δ:  {ch.risingDelta}"),
                        )

                    if ch.mode in ["MF", "MB"]:
                        monitorContent.append(
                            html.P(f"Falling Δ: {ch.fallingDelta}"),
                        )

                    monitorWidgets.append(
                        html.Div(monitorContent, className="gpio-widget input-gpio-widget")
                    )

                plotInFreqGraph = ch.modeSettings.get(ch.mode + "PlotFreqInGraph", False)
                self.frequencyFig.update_layout(
                    yaxis=dict(
                        title = 'Period (s)' if self.plotPeriodInsteadOfFreq else "Frequency (Hz)",
                    ),
                )
                if updateDataInPlot(ch            = ch,
                                    figure        = self.frequencyFig, 
                                    xPoints       = tuple(ch.freqsUpdates), 
                                    yPoints       = tuple(1.0/f if self.plotPeriodInsteadOfFreq else f for f in ch.freqs), 
                                    plotInGraph   = plotInFreqGraph):
                    retFreqGraph = self.frequencyFig

                plotInDutyGraph = ch.modeSettings.get(ch.mode + "PlotDutyCycleInGraph", False)
                if plotInDutyGraph:
                    pass
                if updateDataInPlot(ch            = ch,
                                    figure        = self.dutyCycleFig, 
                                    xPoints       = tuple(ch.freqsUpdates), 
                                    yPoints       = tuple(ch.dutyCycles), 
                                    plotInGraph   = plotInDutyGraph):
                    retDutyGraph = self.dutyCycleFig

                plotInDeltasGraph = ch.modeSettings.get(ch.mode + "PlotDeltasInGraph", False)
                if updateDataInPlot(ch            = ch,
                                    figure        = self.deltaFig, 
                                    xPoints       = tuple(ch.deltas), 
                                    yPoints       = None, 
                                    plotInGraph   = plotInDeltasGraph):
                    retDeltaGraph = self.deltaFig

            self.channelsLock.release()

            return retFreqGraph, retDutyGraph, retDeltaGraph, inputWidgets, outputWidgets, monitorWidgets

        @self.app.callback(
            Output("dummy", "children", allow_duplicate=True), 
            Input({"type": "gpio-set-high", "index": ALL}, "n_clicks"),
            prevent_initial_call=True
        )
        def handleGPIOSetHigh(n_clicks: list[int]):
            if not any(n_clicks):
                # If all are None, then, no click was done.
                raise PreventUpdate
            
            # ctx.triggered_id["index"] = Returns the ID of the triggered button.
            ch = self.config.getChannel(ctx.triggered_id["index"])
            if ch is None:
                self.events.raiseError("Cannot set channel level", "Invalid channel number.")
                return ""
            
            self.events.commandContent += MIDDSParser.encodeOutput(ch.number, 1, 0)
            self.events.commandContent += MIDDSParser.encodeInput(ch.number, 0, 0)
            self.events.sendCommandToMIDDS.set()

        @self.app.callback(
            Output("dummy", "children", allow_duplicate=True), 
            Input({"type": "gpio-set-low", "index": ALL}, "n_clicks"),
            prevent_initial_call=True
        )
        def handleGPIOSetLow(n_clicks):
            if not any(n_clicks):
                # If all are None, then, no click was done.
                raise PreventUpdate

            # ctx.triggered_id["index"] = Returns the ID of the triggered button.
            ch = self.config.getChannel(ctx.triggered_id["index"])
            if ch is None:
                self.events.raiseError("Cannot set channel level", "Invalid channel number.")
                return ""
            
            self.events.commandContent += MIDDSParser.encodeOutput(ch.number, 0, 0)
            self.events.commandContent += MIDDSParser.encodeInput(ch.number, 0, 0)
            self.events.sendCommandToMIDDS.set()

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
            
            raise PreventUpdate

        @self.app.callback(
            Output('settings-div', 'className', allow_duplicate=True),
            Output('settings-div-content', 'children'),
            
            Input('settings-btn', 'n_clicks'),
            Input('discard-settings-btn', 'n_clicks'),
            State('settings-div', 'className'),
            prevent_initial_call=True
        )
        def toggleSettings(c1: int, c2: int, settingsClassName: str):
            settingsDivClassName = no_update
            settingsDivContent = no_update
            if "hidden" in settingsClassName or "initialHidden" in settingsClassName:
                # Populate the settings pane with updated data.
                settingsDivContent = \
                    html.Section([
                        html.Label("SYNC", className="section-title"),
                        html.Div([
                            html.P("Channel"),
                            dcc.Dropdown(
                                id="sync-channel", 
                                options=
                                [{
                                    "label": "Disabled",
                                    "value": -1,
                                }]
                                +
                                [{
                                    "label": f"Channel {i}", 
                                    "value": i
                                } for i in range(0, int(self.config['ProgramConfig']['CHANNEL_COUNT']))], 
                                value=int(self.config['SYNC']['Channel']),
                                clearable=False
                            ),
                            
                            html.P("Frequency (Hz)"),
                            dcc.Input(
                                id="sync-frequency", className="inputPane", 
                                type="number", min=0.01, max=100.0,
                                placeholder="Enter SYNC frequency", 
                                value=float(self.config['SYNC']['Frequency']),
                                disabled=int(self.config['SYNC']['Channel']) == -1
                            ),

                            html.P("Duty Cycle (%)"),
                            dcc.Input(
                                id="sync-duty", className="inputPane", 
                                type="number", min=0.000001, max=99.999999,
                                placeholder="Enter SYNC duty cycle", 
                                value=float(self.config['SYNC']['DutyCycle']),
                                disabled=int(self.config['SYNC']['Channel']) == -1
                            ),

                        ], className="sync-row-content")
                    ],className="sync-row")             
                
                settingsDivClassName = "settings-div"
            else:
                settingsDivClassName = "settings-div hidden"

            return settingsDivClassName, settingsDivContent

        @self.app.callback(
            Output('settings-div', 'className', allow_duplicate=True),
            
            Input('apply-settings-btn', 'n_clicks'),
            State('sync-channel', 'value'),
            State('sync-frequency', 'value'),
            State('sync-duty', 'value'),
            prevent_initial_call=True,
            suppress_callback_exceptions=True
        )
        def applySettings(c1: int, syncCh: str, syncFreq: float|int, syncDuty: float|int):
            if not self.events.deviceConnected:
                self.events.raiseError("Cannot apply settings",
                                       "Connect to MIDDS first by entering the serial port and clicking the 'Connect' button.")
                raise PreventUpdate

            if syncCh != -1:
                selCh = self.config.getChannel(syncCh)
                if selCh is None:
                    self.events.raiseError("Cannot apply SYNC settings",
                                          f"Channel {syncCh} is not valid.")
                    raise PreventUpdate
                elif selCh.mode != "MB":
                    self.events.raiseError("Cannot apply SYNC settings",
                                          f"Channel {syncCh} is not in 'Monitoring Both Edges' mode. "
                                           "Set it, apply the channel configuration and the retry to apply settings.")
                    raise PreventUpdate

            syncFreq = float(syncFreq)
            if syncFreq < 0.01 or syncFreq > 100.0:
                    self.events.raiseError("Cannot apply SYNC settings",
                                          f"SYNC frequency {syncFreq} does not fall in the valid range [0.01, 100] Hz")
                    raise PreventUpdate

            syncDuty = float(syncDuty)
            if syncDuty <= 0 or syncDuty >= 100.0:
                    self.events.raiseError("Cannot apply SYNC settings",
                                          f"SYNC duty cycle {syncDuty} does not fall in the valid range (0, 100) %")
                    raise PreventUpdate

            self.config['SYNC']['Channel']   = str(syncCh)
            self.config['SYNC']['Frequency'] = str(syncFreq)
            self.config['SYNC']['DutyCycle'] = str(syncDuty)
            self.config.saveConfig()

            self.events.applySettings.set()

            # Close the settings window.
            return "settings-div hidden"

        @self.app.callback(
            Output('sync-frequency', 'disabled'),
            Output('sync-duty', 'disabled'),
            
            Input('sync-channel', 'value'),
            prevent_initial_call=True,
            suppress_callback_exceptions=True
        )
        def updateFreqAndDutySYNCEnabled(syncCh: str):
            if syncCh == -1:
                return True, True
            else:
                return False, False

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
