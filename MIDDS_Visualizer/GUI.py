import dash
from dash import dcc, html, no_update
from dash.exceptions import PreventUpdate
from dash.dependencies import Input, Output, State
import plotly.graph_objs as go
from ProgramConfiguration import ProgramConfiguration
import copy
from GUI2BackendEvents import GUI2BackendEvents
import time

class GUI:
    def __init__(self, lock, events: GUI2BackendEvents, config):
        self.channelsLock = lock
        self.events = events
        self.config: ProgramConfiguration = config

        self.temporalConfig: ProgramConfiguration = copy.deepcopy(config)
        self.changesToApply: bool = False

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
        self.app.run(debug=debug)

    def setupLayout(self):
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
                        value=self.config.getChannel(self.selectedChannelNumber).mode),

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
                    dcc.Graph(id='freq-graph'),
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
                html.Button("✖", id="close-error-div", className="close-error-div"),
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
            dcc.Interval(id='interval-component', interval=self.interval, n_intervals=0)
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
            if self.events.recording:
                # The computer is recording and by clicking, the user is requesting to stop.
                self.events.stopRecording.set()
            else:
                self.events.startRecording.set()
        
        @self.app.callback(
            Output("record-btn", "children"),
            Output("record-btn", "className"),
            Input("interval-component", "n_intervals")
        )
        def updateRecordButton(n):
            if self.events.startRecording.is_set() or self.events.stopRecording.is_set():
                return dcc.Loading(id="loading-display", display="show"), "record-btn"

            if self.events.recording:
                return "■ Recording", "record-btn connected"
            else:
                return "⏺ Record", "record-btn"

        # Callback for connecting/disconnecting from the device.
        @self.app.callback(
            Input("connect-btn", "n_clicks"),
            prevent_initial_call=True,
        )
        def toggleConnection(n_clicks):
            if self.events.deviceConnected:
                # The device is connected and by clicking, the user is requesting to disconnect.
                self.events.closeSerialPort.set()
            else:
                self.events.openSerialPort.set()
        
        @self.app.callback(
            Output("connect-btn", "children"),
            Output("connect-btn", "className"),
            Input("interval-component", "n_intervals")
        )
        def updateConnectButton(n):
            if self.events.openSerialPort.is_set() or self.events.closeSerialPort.is_set():
                return dcc.Loading(id="loading-display", display="show"), "connect-btn"

            if self.events.deviceConnected:
                return "Connected", "connect-btn connected"
            else:
                return "Connect", "connect-btn"

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
            Output("apply-config", "className", allow_duplicate=True),

            Input("gpio-channel", "value"),
            Input("gpio-name", "value"),
            Input("gpio-mode", "value"),

            prevent_initial_call=True,
        )
        def updateChannelConfig(channel, name, mode):
            if channel is None or name is None or mode is None:
                raise PreventUpdate
            
            returnName = no_update
            returnMode = no_update
            applyConfigClassName = no_update

            selChannel = self.temporalConfig.getChannel(channel)
            if selChannel is None:
                self.channelsLock.release()
                raise PreventUpdate
            
            if self.selectedChannelNumber == channel:
                # The selected channel is the same, therefore some data has changed in the GUI.
                if selChannel.name != name or selChannel.mode != mode:
                    # Unhide the configuration div by removing the hidden class.
                    applyConfigClassName = "apply-config"

                selChannel.name = name
                selChannel.mode = mode
            else:
                # The selected channel has changed.
                self.selectedChannelNumber = channel
                # Update the GUI with the new channel information.
                returnName = selChannel.name
                returnMode = selChannel.mode

            # apply-config without the hidden.
            return returnName, returnMode, applyConfigClassName

        # Callback for applying the new configuration.
        @self.app.callback(
            Output("apply-config", "className", allow_duplicate=True),
            Input("apply-config-btn", "n_clicks"),
            prevent_initial_call=True,
        )
        def applyNewConfiguration(n_clicks):
            self.channelsLock.acquire()
            self.config = self.temporalConfig
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
            self.temporalConfig = copy.deepcopy(self.config)
            return "apply-config hidden"

        # Callback for GPIO options.
        @self.app.callback(
            Output('gpio-options', 'children'),
            Input('gpio-mode', 'value')
        )
        def update_gpio_options(mode):
            if mode == "IN":
                return html.Div([
                    dcc.Checklist(id="add-to-freq", 
                                  options=[{"label": "Add to frequency graph", "value": "freq"}], 
                                  inline=True)
                ])
            return html.Div()

        # Callback to update input/output/frequency widgets
        @self.app.callback(
            Output('freq-graph', 'figure'),
            Output("input-channels", "children"),
            Output("output-channels", "children"),
            
            Input("interval-component", "n_intervals")
        )
        def updateWidgets(n):
            fig = go.Figure()

            inputWidgets = []
            outputWidgets = []
            minX = 0
            maxX = 0
            minY = 0
            maxY = 0

            self.channelsLock.acquire()
            for ch in self.config.channels:
                title = f"Ch. {ch.number:02}"
                if ch.name != "":
                    title += f": ({ch.name})" 

                if ch.mode == "IN":
                    inputWidgets.append(
                        html.Div([
                            html.P(title),
                            html.P(f"Level: {ch.channelLevel}"),
                            html.P(f"Freq:  {ch.frequency:.4g}")
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

                if ch.mode.startswith("M") or ch.mode == "FR":
                    xPoints = list(ch.freqsUpdates)
                    yPoints = list(ch.freqs)
                    
                    maxX = max(max(xPoints, default=0), maxX)
                    minX = min(min(xPoints, default=0), minX)
                    maxY = max(max(yPoints, default=0), maxY)
                    minY = min(min(yPoints, default=0), minY)

                    fig.add_trace(go.Scatter(
                        x=xPoints,
                        y=yPoints,
                        mode='markers+lines',
                        marker=dict(size=6, color='red', opacity=0.7),
                        name=ch.name
                    ))

            self.channelsLock.release()

            fig.update_layout(
                xaxis_title='Time',
                xaxis=dict(
                    tickformat='%H:%M:%S',
                    range=[minX, maxX]
                ),
                yaxis_title='Frequency (Hz)',
                yaxis=dict(range=[minY, maxY]),
                
                template='plotly_dark',
                margin=dict(l=20, r=20, t=20, b=20)
            )

            return fig, inputWidgets, outputWidgets

        # Callback to update error messages.
        @self.app.callback(
            Output("error-title",       "children"),
            Output("error-date",        "children"),
            Output("error-content",     "children"),
            Output("error-message-div", "className"),

            Input("interval-component", "n_intervals")
        )
        def updateErrors(n):
            if not self.events.newMIDDSError.is_set():
                raise PreventUpdate

            self.events.newMIDDSError.clear()
            # Remove the hidden class from error-message-div.
            return self.events.errTitle, \
                   self.events.errDate.strftime("%m/%d/%Y, %H:%M:%S"), \
                   self.events.errContent, "error-message-div"

    def setupClientCallbacks(self):
        pass