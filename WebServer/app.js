const express = require('express');

const app = express();
app.use(express.json());

const port = 3000;

// CURRENT VALUES 
var currentWindSpeed = undefined
var currentWindDirection = undefined
var currentTemperature = undefined
var currentTemperatureDecimal = undefined
var currentPressure = undefined
var currentHumidity = undefined
var currentHumidityDecimal = undefined
var currentRain = undefined
var currentLuminosity = undefined

// Html content with most recent values
function newHtmlContent() {
	return `
    <!doctype html>
    <html lang="en">

    <head>
        <title>Weather Station</title>
    </head>

    <body>
		<h1 style="color:blue">Weather Station</h1>
		<br>
			<h2>Wind speed: ${currentWindSpeed}</h2>
			<h2>Wind direction: ${currentWindDirection}</h2>
			<h2>Temperature: ${currentTemperature}</h2>
			<h2>Temperature decimal: ${currentTemperatureDecimal}</h2>
			<h2>Pressure: ${currentPressure}</h2>
			<h2>Humidity: ${currentHumidity}</h2>
			<h2>Humidity decimal: ${currentHumidityDecimal}</h2>
			<h2>Rain: ${currentRain}</h2>
			<h2>Luminosity: ${currentLuminosity}</h2>
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
	currentWindDirection = jsonData.windDirection
	currentTemperature = jsonData.temperature
	currentTemperatureDecimal = jsonData.temperatureDecimal
	currentPressure = jsonData.pressure
	currentHumidity = jsonData.humidity
	currentHumidityDecimal = jsonData.humidityDecimal
	currentRain = jsonData.rain
	currentLuminosity = jsonData.luminosity

  	console.log("Received POST:", jsonData)
  	renderPage(res)
});

app.listen(port, () => {
	console.log(`Example app listening at http://localhost:${port}`)
})