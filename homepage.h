String homePagePart1 = F(
  R"=====(
    <!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Environmental</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
        }
        table {
            width: 100%;
            margin: auto;
            border-collapse: collapse;
        }
        th, td {
            border: 1px solid black;
            padding: 10px;
            text-align: center;
            width: 33%; /* Ensures equal width */
        }
        th {
            background-color: #f2f2f2;
        }
        tr {
            font-size: 20px;
        }
    </style>

    <script>
      var socket;

        function initWebSocket(){
          socket = new WebSocket("ws://" + window.location.hostname + "/ws");

          socket.onopen = function() {
          console.log("WebSocket connected");


      };
        socket.onmessage = function(event) {
        var data = JSON.parse(event.data);
          document.getElementById("TempC").innerText = data.TemperatureC;
          document.getElementById("TempF").innerText = data.TemperatureF;
          document.getElementById("humidity").innerText = data.Humidity;
          document.getElementById("pressure").innerText = data.Pressure;
          document.getElementById("altitude").innerText = data.Altitude;
          document.getElementById("ppm").innerText = data.PPM;
          document.getElementById("aqi").innerText = data.AQI;
          document.getElementById("quality").innerText = data.Quality;
           document.getElementById("fanStatus").innerText = data.fan ? "ON" : "OFF";
          

      };
        socket.onclose = function() {
        console.log("WebSocket disconnected, retrying...");
        setTimeout(initWebSocket, 10000);
      };
    }
      function toggleFan() {
      if (socket && socket.readyState === WebSocket.OPEN) {
        socket.send("toggleFan");
      }
    }
     
          window.onload = initWebSocket;
        </script> 
</head>
<body>

    <h1>Smart Environmental Monitoring System</h1>

        <table>
        <tr>
            <th>Temp C</th>
            <th>Temp F</th>
            <th>Humidity</th>
        </tr>
        <tr>
            <td><span id="TempC"></span> °C</td>
            <td><span id="TempF"></span> °F</td>
            <td><span id="humidity"></span> %</td>
        </tr>

        <tr>
            <th>AQI</th>
            <th>Quality</th>
            <th>PPM</th>
        </tr>
        <tr>
            <td><span id="aqi"></span></td>
            <td><span id="quality"></span></td>
            <td><span id="ppm"></span></td>
        </tr>

        <tr>
            <th>Pressure</th>
            <th>Altitude</th>
            <th>Fan Control</th> 
        </tr>
        <tr>
            <td><span id="pressure"></span> hPa</td>
            <td><span id="altitude"></span> m</td>
            <td>
                <span id="fanStatus">OFF</span><br>
                <button onclick="toggleFan()">Toggle Fan</button>
            </td>
        </tr>
    </table>


</body>
</html>

    
  )====="
);
