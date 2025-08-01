import { sendGetRequest, sendPostRequest } from "/scripts/utils.js"
import { ip, webSocket } from "/scripts/sockets.js";

const brightnessSlider1 = document.getElementById("brightness-1");
const brightnessSlider2 = document.getElementById("brightness-2");
const brightnessSlider3 = document.getElementById("brightness-3");

const presetsButton1 = document.getElementById("icon-1");
const presetsButton2 = document.getElementById("icon-2");
const presetsButton3 = document.getElementById("icon-3");

const closePopupButton = document.getElementById("close-popup");

const submitButton1 = document.getElementById("submit-1");
const submitButton2 = document.getElementById("submit-2");
const submitButton3 = document.getElementById("submit-3");

const savePresetButton1 = document.getElementById("heart-1");
const savePresetButton2 = document.getElementById("heart-2");
const savePresetButton3 = document.getElementById("heart-3");

brightnessSlider1.onchange = setBrightness;
brightnessSlider2.onchange = setBrightness;
brightnessSlider3.onchange = setBrightness;

brightnessSlider1.oninput = updateBrightnessValue;
brightnessSlider2.oninput = updateBrightnessValue;
brightnessSlider3.oninput = updateBrightnessValue;

presetsButton1.onclick = openPopup;
presetsButton2.onclick = openPopup;
presetsButton3.onclick = openPopup;

closePopupButton.onclick = closePopup;

submitButton1.onclick = submit;
submitButton2.onclick = submit;
submitButton3.onclick = submit;

savePresetButton1.onclick = savePreset;
savePresetButton2.onclick = savePreset;
savePresetButton3.onclick = savePreset;

function setBrightness() {
    const id = this.id.split('-')[1];
    const newValue = this.value;
    webSocket.send("uv" + id + ":" + newValue);
}

function updateBrightnessValue() {
    const id = this.id.split('-')[1];
    const newValue = this.value;
    const brightness = document.getElementById("brightness-" + id + "-value");
    brightness.textContent = newValue;
}

async function openPopup() {
    const sectionId = this.id.split('-')[1];
    const popup = document.getElementById("popup");
    popup.style.display = "block";

    const template = document.getElementById("template");
    let templatesList = document.getElementById("templates-list");

    let data = {};

    try {
        data = await sendGetRequest("http://" + ip + "/presets/get");
        console.log(JSON.stringify(data));
        for (var preset in data["presets"]) {
            var newNode = template.cloneNode(true);
            newNode.id = "item-" + preset;
            newNode.style.display = "flex";
    
            newNode.querySelector('.pure-u-1-4:nth-child(1) p').textContent = data["presets"][preset]["name"];
            newNode.querySelector('.pure-u-1-6:nth-child(2) p').textContent = data["presets"][preset]["time-on"];
            newNode.querySelector('.pure-u-1-6:nth-child(3) p').textContent = data["presets"][preset]["time-off"];
            newNode.querySelector('.pure-u-1-6:nth-child(4) p').textContent = data["presets"][preset]["brightness"] + "%";
    
            newNode.querySelector('.pure-u-1-6:nth-child(5) button').id = "apply-button-" + preset;
            newNode.querySelector('.pure-u-1-6:nth-child(5) button').onclick = function () {
                applyPreset.call(this, sectionId);
            };
    
            newNode.querySelector('.pure-u-1-6:nth-child(5) img').id = "delete-button-" + preset;
            newNode.querySelector('.pure-u-1-6:nth-child(5) img').onclick = function () {
                deletePreset.call(this, sectionId);
            };
    
            templatesList.appendChild(newNode);
        }
    } catch (error) {
        console.error(error);
        alert("Не удалось получить шаблоны!");
    }

}

function closePopup() {
    document.getElementById("popup").style.display = "none";

    const templatesList = document.getElementById("templates-list");
    let templateNode = null;
    while (templatesList.firstChild) {
        if (templatesList.firstChild.id === "template") {
            templateNode = templatesList.firstChild;
        }
        templatesList.removeChild(templatesList.firstChild);
    }
    templatesList.appendChild(templateNode);
}

async function submit() {
    const id = this.id.split("-")[1];
    let name = document.getElementById("name-" + id).textContent;
    let timeOn = document.getElementById("time-on-" + id).value;
    let timeOff = document.getElementById("time-off-" + id).value;
    let brightness = document.getElementById("brightness-" + id).value;
    if (timeOn === "")
        timeOn = "00:00"
    if (timeOff === "")
        timeOff = "00:00"
    if (name === "")
        name = "-"
    if (name.length > 32)
        name = name.slice(0, 32);
    const data = {
        "name": name,
        "time-on": "1970-01-01 " + timeOn + ":00",
        "time-off": "1970-01-01 " + timeOff + ":00",
        "brightness": Number(brightness)
    };
    const json = JSON.stringify(data);
    try {
        const data = await sendPostRequest("http://" + ip + "/group-" + id + "/apply", json);
        console.log(JSON.stringify(data));
        alert("Значения установлены!");
    } catch (error) {
        console.error(error);
        alert("Ошибка: значения не установлены!");
    }
}

async function savePreset() {
    const id = this.id.split("-")[1];
    let name = document.getElementById("name-" + id).textContent;
    let timeOn = document.getElementById("time-on-" + id).value;
    let timeOff = document.getElementById("time-off-" + id).value;
    const brightness = document.getElementById("brightness-" + id).value;
    if (timeOn === "")
        timeOn = "00:00"
    if (timeOff === "")
        timeOff = "00:00"
    if (name === "")
        name = "-"
    if (name.length > 32)
        name = name.slice(0, 32);
    const data = {
        "name": name,
        "time-on": timeOn,
        "time-off": timeOff,
        "brightness": Number(brightness)
    };
    const json = JSON.stringify(data);
    try {
        const data = await sendPostRequest("http://" + ip + "/preset/save", json);
        console.log(JSON.stringify(data));
        alert("Шаблон сохранен!");
    } catch (error) {
        console.error(error);
        alert("Ошибка, шаблон не сохранен!");
    }
}

async function applyPreset(sectionId) {
    const templateId = this.id.slice(13);

    const name = document.getElementById("item-" + templateId).querySelector('.pure-u-1-4:nth-child(1) p').textContent;
    const timeOn = document.getElementById("item-" + templateId).querySelector('.pure-u-1-6:nth-child(2) p').textContent;
    const timeOff = document.getElementById("item-" + templateId).querySelector('.pure-u-1-6:nth-child(3) p').textContent;
    const brightness = document.getElementById("item-" + templateId).querySelector('.pure-u-1-6:nth-child(4) p').textContent.split("%")[0];
    
    document.getElementById("name-" + sectionId).textContent = name;
    document.getElementById("time-on-" + sectionId).value = timeOn;
    document.getElementById("time-off-" + sectionId).value = timeOff;
    document.getElementById("brightness-" + sectionId).value = brightness;
    document.getElementById("brightness-" + sectionId + "-value").textContent = brightness;

    const data = {
        "name": name,
        "time-on": "1970-01-01 " + timeOn + ":00",
        "time-off": "1970-01-01 " + timeOff + ":00",
        "brightness": Number(brightness)
    };

    closePopup();

    const json = JSON.stringify(data);
    try {
        const data = await sendPostRequest("http://" + ip + "/group-" + sectionId + "/apply", json);
        console.log(JSON.stringify(data));
        alert("Значения установлены!");
    } catch (error) {
        console.error(error);
        alert("Ошибка: значения не установлены!");
    }
}

async function deletePreset(sectionId) {
    const templateId = this.id.slice(14);
    const data = {
        "id": templateId
    };
    const json = JSON.stringify(data);
    try {
        const data = await sendPostRequest("http://" + ip + "/preset/delete", json);
        console.log(JSON.stringify(data));
    } catch (error) {
        console.error(error);
        alert("Ошибка, шаблон не удалён!");
        return;
    }
    const templatesList = document.getElementById("templates-list");
    const childToDelete = templatesList.querySelector("#item-" + templateId);
    if (childToDelete) {
        templatesList.removeChild(childToDelete);
        const children = templatesList.children;
        let countID = 0;
        for (let i = 0; i < children.length; i++) {
            if (children[i].id !== "template") {
                children[i].id = "item-" + String(countID);
                children[i].querySelector('.pure-u-1-6:nth-child(5) img').id = "delete-button-" + countID;
                children[i].querySelector('.pure-u-1-6:nth-child(5) img').onclick = function () {
                    deletePreset.call(this, sectionId);
                };
                countID++;
            }
        }
    }
}