var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function iconFromWeatherId(weatherId) {
  if (weatherId < 600) {
    return 2;
  } else if (weatherId < 700) {
    return 3;
  } else if (weatherId > 800) {
    return 1;
  } else {
    return 0;
  }
}
 
function locationSuccess(pos) {
  // Construct URL
  var url = "http://api.openweathermap.org/data/2.5/weather?lat=" +
      pos.coords.latitude + "&lon=" + pos.coords.longitude + '&cnt=1';
  //
      //pos.coords.latitude + "&lon=" + pos.coords.longitude;
 
  // Send request to OpenWeatherMap
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);
 
      // Temperature in Kelvin requires adjustment
      var temperature = Math.round(json.main.temp - 273.15);
      console.log("Temperature is " + temperature);
 
      // Conditions
      var conditions = json.weather[0].main;      
      console.log("Conditions are " + conditions);
      
      var icon = iconFromWeatherId(json.weather[0].id);
      // 
//      var name = json.coord.name;      
//     console.log("Position are " + name);
      
      
      // Assemble dictionary using our keys
      var dictionary = {
        'WEATHER_ICON_KEY': icon,
        "KEY_TEMPERATURE": temperature,
        "KEY_CONDITIONS": conditions
        //"KEY_NAME": name
      };
 
      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log("Weather info sent to Pebble successfully!");
        },
        function(e) {
          console.log("Error sending weather info to Pebble!");
        }
      );
    }      
  );
}
 
function locationError(err) {
  console.log("Error requesting location!");
}
 
function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}
 
// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log("PebbleKit JS ready!");
 
    // Get the initial weather
    getWeather();
  }
);
 
// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received!");
    getWeather();
  }                     
);