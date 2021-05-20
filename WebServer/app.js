const http = require('http');
const fs = require('fs')

const hostname = '127.0.0.1';
const port = 3000;

const server = http.createServer((req, res) => {
    if (req.method === 'POST') {
        console.log("Received POST")
    }
    if (req.method === "GET") {
        console.log("Received GET")
        req.body
    }
    res.writeHead(200, {'Content-Type': 'text/plain'});
    
    // Write html file
    /*fs.readFile('index.html', function(error, data) {
        if (error) {
            res.writeHead(404)
        } 
        else {
            res.write(data)
        }
    })*/

    res.end("Hello world");
});

server.listen(port, hostname, () => {
  console.log(`Server running at http://${hostname}:${port}/`);
});
