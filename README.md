# TransportCatalogue

TransportCatalogue is a transport directory. It works with JSON requests. In response to a route rendering request, it outputs an SVG format answer. It implements a JSON constructor using a chain of method calls; explicit errors are found at the compilation stage.



## Program <code>make_base</code>
The <code>make_base</code> program receives JSON from the standard input with the following keys:
  - <code>base_requests</code>: an array with requests <code>Bus</code> and <code>Stop</code> to create a database.
  - <code>routing_settings</code>: routing settings.
  - <code>render_settings</code>: rendering settings.
  - <code>serialization_settings</code>: serialization settings. A dictionary with a single key <code>file</code>, which corresponds to a string — the name of the file. The database is serialized to this file.
The <code>make_base</code> program constructs a database based on the input and serializes it to the specified file name.

## Program <code>process_requests</code>
The <code>process_requests</code> program receives JSON from the standard input with the following keys:
  - <code>stat_requests</code>: requests to the existing database.
  - <code>serialization_settings</code>: serialization settings. A dictionary with a single key <code>file</code>, which corresponds to a string - the name of the file. The database is deserialized from this file.

   
## Base Requests
### Request <code>Stop</code>
Description of the stop is a dictionary with the following keys:
  - <code>type</code>: a string that is equal to "Stop".
  - <code>name</code>: the name of the stop.
  - <code>latitude</code> and <code>longitude</code>: the latitude and longitude of the stop (floating point numbers).
  - <code>road_distances</code>: a dictionary that specifies the road distance from this stop to neighboring stops. Each key in this dictionary is the name of a neighboring stop, and the value is the integer distance in meters.

### Request <code>Bus</code>
Description of a bus route is a dictionary with the following keys:
  - <code>type</code>: a string that is equal to "Bus".
  - <code>name</code>: the name of the route.
  - <code>stops</code>: an array of stop names through which the route passes. For a circular route, the name of the last stop duplicates the name of the first. For example:  <code>["stop1", "stop2", "stop3", "stop1"]</code>.
  - <code>is_roundtrip</code>: a boolean value. <code>true</code> if the route is circular.



## Render Settings
The structure of the dictionary <code>render_settings</code>:
  - <code>width</code> and <code>height</code>: the width and height of the image in pixels.
  - <code>padding</code>: the padding of the map edges from the boundaries of the SVG document.
  - <code>line_width</code>: the thickness of the lines used to draw bus routes.
  - <code>stop_radius</code>: the radius of the circles used to mark stops.
  - <code>bus_label_font_size</code>: the size of the text used for the names of bus routes.
  - <code>bus_label_offset</code>: the offset of the route name label relative to the coordinates of the final stop on the map. An array of two floating point numbers. Sets the values of the <code>dx</code> and <code>dy</code> properties of the SVG \<text\> element.
  - <code>stop_label_font_size</code>: the size of the text used to display stop names.
  - <code>stop_label_offset</code>: the offset of the stop name relative to its coordinates on the map. An array of two double type elements. Sets the values of the dx and dy properties of the SVG <text> element.
  - <code>underlayer_color</code>: the color of the background under the names of stops and routes.
  - <code>underlayer_width</code>: the thickness of the background under the names of stops and routes. Sets the value of the stroke-width attribute of the <text> element.
  - <code>color_palette</code>: the color palette. A non-empty array.

A color can be specified in one of the following formats:
  - as a string, for example, "red" or "black";
  - as an array of three integers in the range of [0, 255]. They define the red, green and blue color components in the RGB format.
  - as an array of four elements: three integers in the range [0, 255] and one floating-point number in the range [0.0, 1.0]. They define the red, green, blue, and opacity components of the color in the RGBA format.


## Routing Settings
The structure of the dictionary <code>routing_settings</code>:
  - <code>bus_wait_time</code>: waiting time for the bus at stops, in minutes.
  - <code>bus_velocity</code>: bus speed, in km/h.




## Stat Requests

### Request Bus
<b>Input</b> is a dictionary with the following keys:
  - <code>type</code>: a string that is equal to "Bus".
  - <code>name</code>: the name of the route.
  - <code>id</code>: the ID of the request.

<b>Output</b>:
  - <code>curvature</code>: the curvature of the route.
  - <code>request_id</code>: the ID of the corresponding <code>Bus</code> request.
  - <code>route_length</code>: the length of the route in meters.
  - <code>stop_count</code>: the number of stops on the route.
  - <code>unique_stop_count</code>: the number of unique stops on the route.
  - <code>error_message</code>: an error message if any error occurred.

### Request Stop
<b>Input</b> is a dictionary with the following keys:
  - <code>type</code>: a string that is equal to "Stop".
  - <code>name</code>: the name of the stop.
  - <code>id</code>: the ID of the request.

<b>Output</b>:
  - <code>buses</code>: an array of route names that pass through this stop.
  - <code>request_id</code>: the ID of the corresponding <code>Stop</code> request.
  - <code>error_message</code>: an error message if any error occurred.
    
### Request Map
<b>Input</b> is a dictionary with the following keys:
  - <code>type</code>: a string that is equal to "Map"
  - <code>id</code>: the ID of the request.

<b>Output</b>:
  - <code>map</code>: a string containing a map in SVG format.
  - <code>request_id</code>: the ID of the corresponding <code>Map</code> request.
