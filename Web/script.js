// Variables globales
let client;
let isConnected = false;
let temperatureData = [];
let temperatureChart;

// Elementos DOM
const statusIndicator = document.getElementById("status-indicator");
const statusText = document.getElementById("status-text");
const connectBtn = document.getElementById("connect-btn");
const disconnectBtn = document.getElementById("disconnect-btn");
const temperatureEl = document.getElementById("temperature");
const humidityEl = document.getElementById("humidity");
const messageLog = document.getElementById("message-log");

// Inicializar gráfico
function initChart() {
  const ctx = document.getElementById("temperature-chart").getContext("2d");
  temperatureChart = new Chart(ctx, {
    type: "line",
    data: {
      labels: [],
      datasets: [
        {
          label: "Temperatura °C",
          data: temperatureData,
          borderColor: "#2563eb",
          tension: 0.1,
          fill: false,
        },
      ],
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      scales: {
        y: {
          beginAtZero: false,
        },
      },
    },
  });
}

// Conectar al broker MQTT
function connectToBroker() {
  const host = document.getElementById("broker-host").value;
  const port = parseInt(document.getElementById("broker-port").value);
  const clientId =
    document.getElementById("client-id").value +
    Math.random().toString(16).substring(2, 8);

  client = new Paho.MQTT.Client(host, port, clientId);

  client.onConnectionLost = onConnectionLost;
  client.onMessageArrived = onMessageArrived;

  const options = {
    timeout: 3,
    onSuccess: onConnect,
    onFailure: onFailure,
    useSSL: true,
  };

  client.connect(options);
}

// Callback de conexión exitosa
function onConnect() {
  isConnected = true;
  updateConnectionStatus(true);
  addMessageToLog("Conectado al broker MQTT");

  // Suscribirse a los temas
  client.subscribe("esp32/temperature");
  client.subscribe("esp32/humidity");
  client.subscribe("esp32/status");
  addMessageToLog(
    "Suscrito a temas: esp32/temperature, esp32/humidity, esp32/status"
  );
}

// Callback de fallo de conexión
function onFailure() {
  addMessageToLog("Error al conectar con el broker");
  updateConnectionStatus(false);
}

// Callback de pérdida de conexión
function onConnectionLost(responseObject) {
  if (responseObject.errorCode !== 0) {
    addMessageToLog("Conexión perdida: " + responseObject.errorMessage);
    updateConnectionStatus(false);
  }
}

// Callback de mensaje recibido
function onMessageArrived(message) {
  addMessageToLog(
    "Mensaje recibido: [" +
      message.destinationName +
      "] " +
      message.payloadString
  );

  // Procesar según el topic
  switch (message.destinationName) {
    case "esp32/temperature":
      const temp = parseFloat(message.payloadString);
      temperatureEl.textContent = temp.toFixed(1);

      // Actualizar gráfico
      temperatureData.push(temp);
      if (temperatureData.length > 15) temperatureData.shift();

      temperatureChart.data.labels = Array.from(
        { length: temperatureData.length },
        (_, i) => i + 1
      );
      temperatureChart.data.datasets[0].data = temperatureData;
      temperatureChart.update();
      break;

    case "esp32/humidity":
      humidityEl.textContent = parseFloat(message.payloadString).toFixed(1);
      break;

    case "esp32/alarma":
      // Procesar estado del dispositivo
      break;
  }
}

// Publicar mensaje
function publishMessage(topic, message) {
  if (!isConnected) {
    addMessageToLog("Error: No conectado al broker");
    return;
  }

  const mqttMessage = new Paho.MQTT.Message(message);
  mqttMessage.destinationName = topic;
  client.send(mqttMessage);
  addMessageToLog("Mensaje enviado: [" + topic + "] " + message);
}

// Actualizar estado de conexión en UI
function updateConnectionStatus(connected) {
  isConnected = connected;

  if (connected) {
    statusIndicator.classList.remove("disconnected");
    statusIndicator.classList.add("connected");
    statusText.textContent = "Conectado";
  } else {
    statusIndicator.classList.remove("connected");
    statusIndicator.classList.add("disconnected");
    statusText.textContent = "Desconectado";
  }
}

// Añadir mensaje al log
function addMessageToLog(message) {
  const now = new Date();//Esto se deberuia cambiar por la hora del RTC del esp32
  const timeString = now.toLocaleTimeString();
  const messageElement = document.createElement("p");
  messageElement.textContent = `[${timeString}] ${message}`;
  messageLog.appendChild(messageElement);
  messageLog.scrollTop = messageLog.scrollHeight;
}

// Event Listeners
connectBtn.addEventListener("click", function () {
  addMessageToLog("Conectando al broker...");
  connectToBroker();
});

disconnectBtn.addEventListener("click", function () {
  if (client && isConnected) {
    client.disconnect();
    updateConnectionStatus(false);
    addMessageToLog("Desconectado del broker");
  }
});

// Controles de dispositivos
document.getElementById("led-on").addEventListener("click", function () {
  publishMessage("esp32/relay_luz", "ON");
});

document.getElementById("led-off").addEventListener("click", function () {
  publishMessage("esp32/relay_luz", "OFF");
});

//Falta ver como imlpementamos la configuracion de la alarma

document.getElementById("alarma-on").addEventListener("click", function () {
  publishMessage("esp32/alarma", "ON");
});

document.getElementById("alarma-off").addEventListener("click", function () {
  publishMessage("esp32/alarma", "OFF");
});

// Inicializar la aplicación
window.onload = function () {
  initChart();
  updateConnectionStatus(false);
};
