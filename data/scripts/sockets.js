import * as utils from"/scripts/utils.js";

export const ip = window.location.hostname;
export let webSocket;

window.addEventListener("load", async function pageLoad() {
    webSocket = initWebSocketConnection(ip);
    let response = {};
    try {
        response = await utils.syncTime(ip);
        console.log(JSON.stringify(response));
    } catch (error) {
        console.error(error);
        alert("Ошибка синхронизации времени!");
    }
});

function initWebSocketConnection(_ip) {
    console.log("Initiating WebSocket connection...");
    webSocket = new WebSocket("ws://" + _ip + "/ws");
    webSocket.onopen = handleWebSocketOpen;
    webSocket.onclose = handleWebSocketClose;
    webSocket.onerror = handleWebSocketError;
    webSocket.onmessage = handleWebSockeMessage;

    return webSocket;
}

function handleWebSocketOpen() {
    console.log("WebSocket connection opened!");

    webSocket.send("get-temperature");
    webSocket.send("get-humidity");
    webSocket.send("get-water-level");

    webSocket.send("get-uv1-status");
    webSocket.send("get-uv2-status");
    webSocket.send("get-uv3-status");

    webSocket.send("get-uv1-value");
    webSocket.send("get-uv2-value");
    webSocket.send("get-uv3-value");

    webSocket.send("get-name-1");
    webSocket.send("get-name-2");
    webSocket.send("get-name-3");

    webSocket.send("get-time-on-1");
    webSocket.send("get-time-on-2");
    webSocket.send("get-time-on-3");

    webSocket.send("get-time-off-1");
    webSocket.send("get-time-off-2");
    webSocket.send("get-time-off-3");
}

function handleWebSocketClose() {
    console.log("WebSocket connection closed.");
    webSocket = initWebSocketConnection(ip);
}

function handleWebSocketError(error) {
    console.log("WebSocket connection error:", error);
}

function handleWebSockeMessage(event) {
    const message = event.data;

    if (message.includes("temperature:")) {
        const value = message.slice(12);
        utils.updateTemperature(value);
    }
    else if (message.includes("humidity:")) {
        const value = message.slice(9);
        utils.updateHumidity(value);
    }
    else if (message.includes("water-level:")) {
        const value = message.slice(12);
        utils.updateWaterLevel(value);
    }
    else if (message.includes("uv1-status:")) {
        const value = message.slice(11);
        const uvId = message.slice(2, 3);
        utils.updateUvStatus(uvId, value);
    }
    else if (message.includes("uv2-status:")) {
        const value = message.slice(11);
        const uvId = message.slice(2, 3);
        utils.updateUvStatus(uvId, value);
    }
    else if (message.includes("uv3-status:")) {
        const value = message.slice(11);
        const uvId = message.slice(2, 3);
        utils.updateUvStatus(uvId, value);
    }
    else if (message.includes("uv1-value:")) {
        const value = message.slice(10);
        const uvId = message.slice(2, 3);
        utils.updateUvValue(uvId, value);
    }
    else if (message.includes("uv2-value:")) {
        const value = message.slice(10);
        const uvId = message.slice(2, 3);
        utils.updateUvValue(uvId, value);
    }
    else if (message.includes("uv3-value:")) {
        const value = message.slice(10);
        const uvId = message.slice(2, 3);
        utils.updateUvValue(uvId, value);
    }
    else if (message.includes("name-1:")) {
        const value = message.slice(7);
        const nameId = message.slice(5, 6);
        utils.updateName(nameId, value);
    }
    else if (message.includes("name-2:")) {
        const value = message.slice(7);
        const nameId = message.slice(5, 6);
        utils.updateName(nameId, value);
    }
    else if (message.includes("name-3:")) {
        const value = message.slice(7);
        const nameId = message.slice(5, 6);
        utils.updateName(nameId, value);
    }
    else if (message.includes("time-on-1:")) {
        const value = message.slice(10);
        const timeId = message.slice(8, 9);
        utils.updateTimeOn(timeId, value);
    }
    else if (message.includes("time-on-2:")) {
        const value = message.slice(10);
        const timeId = message.slice(8, 9);
        utils.updateTimeOn(timeId, value);
    }
    else if (message.includes("time-on-3:")) {
        const value = message.slice(10);
        const timeId = message.slice(8, 9);
        utils.updateTimeOn(timeId, value);
    }
    else if (message.includes("time-off-1:")) {
        const value = message.slice(11);
        const timeId = message.slice(9, 10);
        utils.updateTimeOff(timeId, value);
    }
    else if (message.includes("time-off-2:")) {
        const value = message.slice(11);
        const timeId = message.slice(9, 10);
        utils.updateTimeOff(timeId, value);
    }
    else if (message.includes("time-off-3:")) {
        const value = message.slice(11);
        const timeId = message.slice(9, 10);
        utils.updateTimeOff(timeId, value);
    }
}