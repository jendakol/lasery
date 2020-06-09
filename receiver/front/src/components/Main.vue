<template>
    <v-container fluid ma-0 pa-0 fill-height>
        <v-row>
            <v-col/>
            <v-col :cols="8">
                <v-card class="mx-auto px-2" max-width="1000" outlined :elevation="16">
                    <div class="ind" :style="theStyle"></div>
                    <v-card-actions>
                        <v-row>
                            <v-btn @click="initWs()" :disabled="connected">CONNECT</v-btn>
                            <v-btn @click="disconnect()" :disabled="!connected">DISCONNECT</v-btn>
                        </v-row>
                        <v-row>
                            Connected clients: {{ connectedClients }}
                        </v-row>
                        <v-row>
                            <v-btn @click="arm" :disabled="!connected || (armed || alerting)">ARM</v-btn>
                            <v-btn @click="unarm" :disabled="!connected || (!armed)">UNARM</v-btn>
                            <v-btn @click="alert" :disabled="!connected || (!armed || alerting)">ALERT</v-btn>
                        </v-row>
                    </v-card-actions>
                </v-card>
            </v-col>
            <v-col/>
        </v-row>
    </v-container>
</template>

<style>
    div.ind {
        display: block;
        width: 100%;
        height: 300px;
        background-color: #444444;
        margin-top: 10px;
    }
</style>

<script>
    export default {
        name: 'Main',
        mounted() {
        },
        data() {
            return {
                ws: null,
                connected: false,
                armed: false,
                alerting: false,
                connectedClients: 0,
                theStyle: {},
            }
        },
        methods: {
            initWs: function () {
                // let hostname = window.location.hostname;
                let hostname = "192.168.1.18"

                console.log("Connecting to ws://" + hostname + "/frontend")
                this.ws = new WebSocket("ws://" + hostname + "/frontend");

                this.ws.onopen = () => {
                    this.connected = true;
                    this.sendData({op: "init"});
                };

                this.ws.onerror = (err) => {
                    this.connected = false;
                    console.log("WS error: " + err);
                    this.restartConnection()
                };

                this.ws.onmessage = (data) => {
                    this.receiveData(JSON.parse(data.data));
                };

                this.ws.onclose = () => {
                    if (this.connected) {
                        this.restartConnection();
                    } else {
                        console.log("Connection closed");
                    }
                };

                window.addEventListener('beforeunload', () => {
                    console.log("Closing the page");
                    if (this.ws != null && this.ws.readyState === 1) {
                        this.disconnect()
                    }
                }, false)
            },
            restartConnection: function () {
                setTimeout(() => {
                    console.log("Connection closed, restarting");
                    this.initWs();
                }, 1000);
            },
            disconnect: function () {
                this.connected = false;

                if (this.ws != null)
                    this.ws.close();
            },
            sendData: function (json) {
                console.log("Sending: " + JSON.stringify(json));
                if (this.ws != null) this.ws.send(JSON.stringify(json));
            },
            receiveData: function (data) {
                console.log('Message from receiver ', data);

                switch (data.type) {
                    case "state": {
                        this.connectedClients = data.clients;
                        switch (data.state) {
                            case 0: {
                                this.armed = false;
                                this.alerting = false;
                            }
                                break;
                            case 1: {
                                this.armed = true;
                                this.alerting = false;
                            }
                                break;
                            case 2: {
                                this.armed = true;
                                this.alerting = true;
                            }
                                break;
                        }

                        let bgColor = this.armed ? (this.alerting ? "red" : "green") : "grey";
                        this.theStyle = {
                            backgroundColor: bgColor
                        }
                    }
                        break;
                }
            },
            arm: function () {
                this.sendData({op: "arm"});
            },
            unarm: function () {
                this.sendData({op: "unarm"});
            },
            alert: function () {
                this.sendData({op: "alert"});
            }
        }
    }
</script>
