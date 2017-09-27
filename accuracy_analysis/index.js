/**
 * Created by Valentin on 8/3/17.
 */
var W3CWebSocket = require('websocket').w3cwebsocket;
var stringify = require('json-stringify-safe');
var movies = {};
var g_genre = {
    success: 0,
    total: 0,
    errors: 0
}
var g_nationality = {
    success: 0,
    total: 0,
    errors: 0
}

var socket = new W3CWebSocket("ws://localhost:8090");
socket.onerror = function () {
    console.log("An error occured.");
}
socket.onclose = function () {
    console.log("The connection was closed by the server.");
}
socket.onmessage = function (evt) {
    try {
        var prediction = JSON.parse(evt.data);

        var genres = Object.keys(prediction.genre[0]);
        if (movies[prediction.query].genre.indexOf(genres[0]) !== -1)
            ++g_genre.success;
        else if (genres.length >= 2 && movies[prediction.query].genre.indexOf(genres[1]) !== -1)
            g_genre.success += 0.5;
        ++g_genre.total;
    } catch (err) {
        ++g_genre.errors;
    }
    try {
        var nationalities = Object.keys(prediction.nationality[0]);
        if (movies[prediction.query].nationalities.indexOf(" "+nationalities[0]) !== -1)
            ++g_nationality.success;
        else if (nationalities.length >= 2 && movies[prediction.query].nationalities.indexOf(" "+nationalities[1]) !== -1)
            g_nationality.success += 0.5;
        ++g_nationality.total;
    } catch (err) {
        ++g_nationality.errors;
    }
}

socket.onopen = function() {
    var lineReader = require('readline').createInterface({
        input: require('fs').createReadStream('../db/fulldump.json')
    });
    lineReader.on('line', function (line) {
        var movie = JSON.parse(line);
        //if (!movie.trailer) return; Uncomment to use only movies with a trailer as a test dataset
        movies[movie.title] = movie;
        socket.send('a'+movie.title);
    });
    lineReader.on('close', function () {
        setTimeout(function () {
            console.log("=== GENRE ===");
            console.log("Successful predictions: " + g_genre.success);
            console.log("Total attempts: " + g_genre.total);
            console.log("Null results: " + g_genre.errors);
            console.log("Accuracy rate: " + g_genre.success / g_genre.total);

            console.log("=== NATIONALITIES ===");
            console.log("Successful predictions: " + g_nationality.success);
            console.log("Total attempts: " + g_nationality.total);
            console.log("Null results: " + g_nationality.errors);
            console.log("Accuracy rate: " + g_nationality.success / g_nationality.total);
        }, 10000);
    });
}