
# This document describes the json key name that will be used by the microcontroller to ArduPlot *OLD!!! NEED TO CHANGE*
## Features needed

1. Choose type (int, float, bool, string) + arrays
1. Choose graph type (line, bar, heatmap)
1. Insert id, to persist additional graph customizations (line type, color...), and to merge data from multiple sensors
1. Have a human readable name
1. Option for log-only data !!!

## Recurring messages

id: hex 16 bit number (65536 possible ids)
type:

``` txt
n: number
b: boolean
s: string // Log
(+ a if array type: na -> number array)
graph type:
l: line
b: bar
h: heatmap (+ number of pixels in row * number of pixels in column)
t: text
name preceded by ">": >Graph name
```

If selected graph is line, bar, or heatmap, the range can be specified, using 2 int16 values in hex, one after the other, first minimum then maximum

``` txt
min>max>
0000ffff
```

Complete example:

``` json
{
  "0000nb>Int sensor name": 10,
  " ": true
}
```
