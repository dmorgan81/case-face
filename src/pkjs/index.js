var Clay = require('pebble-clay');
var config = require('./config.json');
var customClay = require('./custom-clay');
var clay = new Clay(config, customClay);

var GenericWeather = require('pebble-generic-weather');
var genericWeather = new GenericWeather();

var GeocodeMapquest = require('pebble-geocode-mapquest');
var geocodeMapquest = new GeocodeMapquest();

Pebble.addEventListener('appmessage', function(e) {
    genericWeather.appMessageHandler(e);
    geocodeMapquest.appMessageHandler(e);
});

Pebble.addEventListener('ready', function() {
    Pebble.sendAppMessage({ 'APP_READY' : 1 });
});
