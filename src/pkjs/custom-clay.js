module.exports = function(minified) {
    var Clay = this;
    var _ = minified._;
    var $ = minified.$;
    var HTML = minified.HTML;

    Clay.on(Clay.EVENTS.AFTER_BUILD, function() {
        var platform = Clay.meta.activeWatchInfo.platform || 'diorite';

        if (platform != 'aplite') {
            var gpsToggle = Clay.getItemByMessageKey('WEATHER_USE_GPS');
            var locationInput = Clay.getItemByMessageKey('WEATHER_LOCATION_NAME');
            gpsToggle.on('change', function() {
                if (gpsToggle.get()) locationInput.hide();
                else locationInput.show();
            }).trigger('change');
        }

        var widgets = Clay.getItemsByGroup('widget');
        widgets.forEach(function(widget) {
            widget.on('change', function() {
                widgets
                    .filter(function(w) { return w !== this; }, this)
                    .filter(function(w) { return w.get() !== '0'; })
                    .filter(function(w) { return w.get() === this.get(); }, this)
                    .forEach(function(w) { w.set('0'); });
            });
        });

        if (platform == 'aplite') {
            $('optgroup[label="Health"]').each(function(obj) {
                obj['parentNode'].removeChild(obj);
            });
        } else if (platform == 'basalt') {
            $('option[value="8"]').each(function(obj) {
                obj['parentNode'].removeChild(obj);
            })
        }
    });
}
