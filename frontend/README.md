# FlyPrecise Dashboard - Front End

This is the front end for the fueling dashboard. It's just HTML and CSS, no
JavaScript (that was one of the rules). The layout follows the sketch we made:
three tabs (NOW, PAST FLIGHTS, ANALYTICS), and the NOW tab shows the flight,
fuel requested, live density, gallons loaded, and the FlyPrecise difference.

To look at it, open `index.html` in a browser. The links between pages work
so you can click around. All the numbers are placeholder data right now (the
245,850 lb flight from the sketch). The backend will swap in real values.

## Pages

- `index.html` - NOW tab. Tab bar, live readings, the "Load a flight" form,
  START and STOP buttons.
- `live.html` - the live readings box that sits inside index. It reloads
  itself every second so the numbers update without the whole page flashing.
- `past.html` - PAST FLIGHTS tab. One row per fueling, newest first. Each row
  links to its fuel slip.
- `analytics.html` - ANALYTICS tab. Four summary tiles, a bar chart of the
  difference per flight (done with CSS widths), and a density table.
- `slip.html` - the fuel slip. White ticket style, signature lines, prints on
  letter paper or receipt paper.
- `settings.html` - clock, units, planning density, calibration numbers,
  hotspot password.
- `advanced.html` - manual pump/valve toggles behind a warning. Not in the
  tab bar on purpose.
- `style.css` - all the styling. Colors are variables at the top so they're
  easy to change. Uses system fonts since there's no internet at the demo.
- `manifest.json` and `icon.svg` - lets you add it to a phone home screen
  with the FlyPrecise icon.

## For whoever does the backend

The pages are the template. The backend just needs to fill in the values and
handle the forms. Keep the class names the same and the CSS keeps working.

Values to fill in:

- **live.html** - flight number, route, aircraft, the status pill (STANDBY /
  PRIMING / FUELLING / NO LINK), the pump/valve dots, the four big numbers
  (fuel requested, density, loaded, difference), and the progress bar. The
  warning banners at the top only show when something is actually wrong.
- **index.html** - the message after you submit a form (started, stopped,
  error, etc).
- **past.html** - one table row per fueling.
- **analytics.html** - the four tile numbers, one bar per flight, one table
  row per flight.
- **slip.html** - everything on the slip for one fueling.
- **settings.html** - current settings into the inputs.
- **advanced.html** - ON/OFF for each output.

Forms:

- Load a flight: POST to `/start` with `flight`, `acft`, `from`, `to`, `fuel`
- STOP: POST to `/stop`
- Settings: POST to `/settings` with `dt`, `um`, `uv`, `nd`, `ca`, `cb`, `ap`
- Advanced: POST to `/advanced` with `cmd` = P, B, or M

After a form post, redirect back to the page so refreshing doesn't resubmit.

## FlyPrecise difference

```
difference = fuel requested x (1 - measured density / planning density)
```

Planning density defaults to 6.7 lb/gal. With the sketch numbers (245,850 lb
requested, 6.91 lb/gal measured) that comes out to -7,712 lb. Negative means
less fuel actually went on than a density-assumed load would have put on.
