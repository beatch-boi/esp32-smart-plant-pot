function mapValue(x, in_min, in_max, out_min, out_max) {
    return Math.round((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
}

export function updateTemperature(newValue) {
    const temperature = document.getElementById("temperature");
    temperature.textContent = newValue;
}

export function updateHumidity(newValue) {
    const humidity = document.getElementById("humidity");
    humidity.textContent = newValue;
}

export function updateWaterLevel(newValue) {
    const waterLevel = document.getElementById("water-level");
    if (newValue === "0")
        waterLevel.textContent = "Уровень воды в норме";
    else
        waterLevel.textContent = "Уровень воды ниже нормы";
}

export function updateUvStatus(uvId, newStatus) {
    const uvStatus = document.getElementById("status-" + uvId);
    if (newStatus === "on") {
        uvStatus.textContent = "Активен";
        uvStatus.style.color = "green";
    } else if (newStatus === "off") {
        uvStatus.textContent = "Неактивен";
        uvStatus.style.color = "gray";
    }
}

export function updateUvValue(uvId, newValue) {
    const uvSlider = document.getElementById("brightness-" + uvId);
    const uvValue = document.getElementById("brightness-" + uvId + "-value");
    const mapedValue = mapValue(newValue, 0, 255, 0, 100);
    uvSlider.value = mapedValue;
    uvValue.textContent = mapedValue;
}

export function updateName(nameId, newName) {
    const name = document.getElementById("name-" + nameId);
    name.textContent = newName;
}

export function updateTimeOn(timeId, newValue) {
    const time = document.getElementById("time-on-" + timeId);
    time.value = newValue;
}

export function updateTimeOff(timeId, newValue) {
    const time = document.getElementById("time-off-" + timeId);
    time.value = newValue;
}

export async function sendPostRequest(_url, _data) {
    try {
        const response = await fetch(_url, {
            method: "POST",
            headers: {
                'Content-Type': 'application/json',
            },
            body: _data,
        });
        if (!response.ok) {
            const error = await response.json();
            throw error;
        }
        const responseData = await response.json();
        return responseData;
    } catch (error) {
        throw error; // re-throw the error so it can be caught by the calling function
    }
}

export async function sendGetRequest(_url) {
    try {
        const response = await fetch(_url);
        if (!response.ok) {
            const error = await response.json();
            throw error;
        }
        const data = await response.json();
        return data;
    } catch (error) {
        throw error; // re-throw the error so it can be caught by the calling function
    }
}

export async function syncTime(_ip) {
    const date = new Date();

    let hh = date.getHours();
    let mm = date.getMinutes();
    let ss = date.getSeconds();
    let YY = date.getFullYear().toString();
    let MM = date.getMonth() + 1;
    let DD = date.getDate();

    MM = (MM < 10) ? "0" + MM : MM;
    DD = (DD < 10) ? "0" + DD : DD;
    hh = (hh < 10) ? "0" + hh : hh;
    mm = (mm < 10) ? "0" + mm : mm;
    ss = (ss < 10) ? "0" + ss : ss;

    const formattedTime = hh + ":" + mm + ":" + ss;
    const formattedDate = YY + "-" + MM + "-" + DD;
    const data = {
        "datetime": formattedDate + " " + formattedTime
    };

    const json = JSON.stringify(data);
    try {
        const response = await sendPostRequest("http://" + _ip + "/time/synchronize", json);
        if (response["message"] !== "Ok") {
            const error = await response;
            throw error;
        }
        const data = await response;
        return data;
    } catch (error) {
        throw error; // re-throw the error so it can be caught by the calling function
    }
}