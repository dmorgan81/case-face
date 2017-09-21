module.exports = function(minified) {
    var Clay = this;
    var _ = minified._;
    var $ = minified.$;
    var HTML = minified.HTML;

    Clay.on(Clay.EVENTS.AFTER_BUILD, function() {
        var watchInfo = Clay.meta.activeWatchInfo;

        if (watchInfo.platform != 'aplite') {
            var gpsToggle = Clay.getItemByMessageKey('WEATHER_USE_GPS');
            var locationInput = Clay.getItemByMessageKey('WEATHER_LOCATION_NAME');
            gpsToggle.on('change', function() {
                if (gpsToggle.get()) locationInput.hide();
                else locationInput.show();
            }).trigger('change');
        }
    });
}
