const http = require('http');
const fs = require('fs')

const hostname = '127.0.0.1';
const port = 3000;

// CURRENT VALUES 
var currentWindSpeed = -1
var currentTemperature = -1

// Html content with most recent values
function newHtmlContent() {
    return `
    <!doctype html>
    <html lang="en">

    <head>
        <title>Weather Station</title>
    </head>

    <body>
        <h1>Wind speed: ${currentWindSpeed}</h1>
        <h1>Temperature: ${currentTemperature}</h1>
    </body>

    </html>
    `
}

// Configure server responses
const server = http.createServer((req, res) => {
    if (req.method === 'POST') {
        console.log("Received POST")

        var jsonString = ''
        req.on('data', function (data) {
            jsonString += data
        })
        req.on('end', function() {
            var jsonData = JSON.parse(jsonString)

            var windSpeed = jsonData.windSpeed
            var temperature = jsonData.temperature
            console.log('Wind speed: \{windSpeed}')

            currentWindSpeed = windSpeed
            currentTemperature = temperature
        })
    }
    if (req.method === "GET") {
        console.log("Received GET")
    }

    // Render page
    res.writeHead(200, {'Content-Type': 'text/html'});
    res.write(newHtmlContent())
    res.end()
});

// Start listening
server.listen(port, hostname, () => {
  console.log(`Server running at http://${hostname}:${port}/`);
});


