const express = require('express');

const app = express();
app.use(express.json());

const port = 3000;

// CURRENT VALUES 
var currentWindSpeed = undefined
var currentTemperature = undefined
var currentPressure = undefined

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
        <h1>Pressure: ${currentPressure}</h1>
    </body>

    </html>
    `
}

function renderPage(res) {
	// Render page
	res.writeHead(200, {'Content-Type': 'text/html'});
	res.write(newHtmlContent())
	res.end()
}

app.get('/', (req, res) => {
  	console.log("Received GET")
 	renderPage(res)
})

app.post('/', (req, res) => {

	// Load values from JSON
	var jsonData = req.body
	currentWindSpeed = jsonData.windSpeed
	currentTemperature = jsonData.temperature
	currentPressure = jsonData.pressure

  	console.log("Received POST:", jsonData)
  	renderPage(res)
});

app.listen(port, () => {
	console.log(`Example app listening at http://localhost:${port}`)
})