
# This document describes the json key name that will be used by the microcontroller to ArduPlot

## Features needed

1. Choose type (int, float, bool, string) + arrays
2. Choose graph type (line, bar, heatmap)
3. Insert id, to persist additional graph customizations (line type, color...), and to merge data from multiple sensors
4. Have a human readable name

## Proposed solution

id: hex 16 bit number (65536 possible ids)
type:
n: number
b: boolean
s: string
(+ a if array type: na -> number array)
graph type:
l: line
b: bar
h: heatmap (+ number of pixels in row * number of pixels in column)
t: text
name preceded by ">": >Graph name

Complete example:

``` json
{
  "0000nb>Int sensor name": 10,
  " ": true
}
```
