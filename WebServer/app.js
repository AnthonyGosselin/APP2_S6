const http = require('http');
const fs = require('fs')

const hostname = '127.0.0.1';
const port = 3000;

var currentWindSpeed = -1

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
            console.log('Wind speed: \{windSpeed}')
            currentWindSpeed = windSpeed
        })
    }
    if (req.method === "GET") {
        console.log("Received GET")
    }

    res.writeHead(200, {'Content-Type': 'text/html'});

    // Write html file
    fs.readFile('./WebServer/index.html', null, function(error, data) {
        console.log("Get index")
        if (error) {
            console.log(error)
            res.writeHead(404)
            res.end()
        } 
        else {
            //res.write(data)
            //document.getElementById('windspeed').innerHTML = currentWindSpeed

            
            res.write(`
                    <!doctype html>
                    <html lang="en">

                    <head>
                        <title>Hello, world2!</title>
                    </head>

                    <body>
                        <h1>Wind speed:\{currentWindSpeed}</h1>
                    </body>

                    </html>
            `)
            res.end()
        }
    })

    var returnResult = "Current wind speed: \{currentWindSpeed}"
});

server.listen(port, hostname, () => {
  console.log(`Server running at http://${hostname}:${port}/`);
});


