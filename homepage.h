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
        }
        th {
            background-color: #f2f2f2;
           
        }

        tr{
          font-size: 20px;
        }
    </style>
    <script> 
          function fetchTempC() { 
            fetch('/TempC') 
              .then(response => response.text()) 
              .then(tempC => { 
                console.log("Temp C:", tempC); 
                document.getElementById("tempCValue").innerText = tempC; 
              }) 
              .catch(error => console.error('Error fetching temperature:', error)); 
          } 
          setInterval(fetchTempC, 1000); 
          window.onload = fetchTempC; 

          function fetchTempF() { 
            fetch('/TempF') 
              .then(response => response.text()) 
              .then(tempF => { 
                console.log("Temp F:", tempF); 
                document.getElementById("tempFValue").innerText = tempF; 
              }) 
              .catch(error => console.error('Error fetching temperature:', error)); 
          } 
          setInterval(fetchTempF, 1000); 
          window.onload = fetchTempF; 
        </script> 
</head>
<body>

    <h1>Smart Environmental Monitoring System</h1>

    <table>
        <tr>
            <th>Temp C</th>
            <th>Temp F</th>
            <th>Pressure</th>
            <th>Altitude</th>
        </tr>
        <tr>
            <td><span id="tempCValue"></span> °C</td>
            <td> <span id="tempFValue"></span> °F</td>
            <td>Reading hPa</td>
            <td>Reading m</td>
        </tr>
    </table>

</body>
</html>

    
  )====="
);
