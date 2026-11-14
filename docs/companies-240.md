# 240 companies

Company IDs 0 through 239 are playable. Town, unowned, water and script ownership
use IDs 240 through 243. Network joining and spectator IDs remain 253 through 255.

Set `network.max_companies` to `240` in the server configuration, or use
`setting network.max_companies 240` in the console. The existing client limit
of 255 is sufficient for one client in each company; spectators and additional
clients in a company also count towards that client limit.

New companies choose from the least-used standard palette colours. Company numbers
and names remain available to distinguish companies with the same colour.

## 24-bit company colours

Open your company window, choose **Colour scheme**, then click **Primary hex colour**
or **Secondary hex colour**. Enter six hexadecimal digits, such as `#E57A19` or
`126BDF`. Black (`000000`) and white (`FFFFFF`) are valid. Leave the entry empty to
restore the selected standard palette colour. Choosing a standard colour in the
default scheme also clears that channel's RGB override.

RGB colours apply to the default company livery, vehicles using its colours,
company icons, property, graph lines and legend swatches. Company title bars and
owned sign/station/waypoint labels also use RGB, including transparent label text;
dark and light backgrounds select readable text colours. Vehicle schemes and groups
with different standard colours keep those colours. A 32-bit or 40-bit blitter
displays the full RGB value, with shading for sprite details. An 8-bit blitter,
legacy NewGRF colour variables and server admin colour fields retain the standard
palette fallback. The minimap's indexed terrain pixels also use that fallback.

The console and dedicated-server RCON support the same networked colour command.
Company numbers here are **1 through 240**:

```text
company_colour_hex 240 primary E57A19
company_colour_hex 240 secondary 126BDF
company_colour_hex 240
company_colour_hex 240 primary default
```

Clients can change their own company's colours. The server can change any company.
Queries show the stored primary and secondary overrides.

## Storage and compatibility

* Company masks contain fifteen 16-bit words, covering exactly 240 companies.
* Each tile uses two additional bytes for the high tile, road and tram owner bits.
  Existing tile flags and road types retain their original positions.
* Whole-map saves use format version 3. Scenario saves use separate `MAP9` and
  `MAPX` owner chunks. The mandatory `companies_240` feature identifies the format.
* Existing standard JGRPP saves retain their original company IDs. Loading converts
  special owner IDs, town arrays, engine masks and cargo-monitor keys.
* Saves made with this extension require a build that understands `companies_240`.
  Earlier experimental more-companies saves do not carry this feature marker and
  are a separate save format.
* RGB values are saved under the additional mandatory `company_rgb24` feature.
  This build loads earlier 240-company saves with their standard colours. New saves
  require RGB support to load.
* All players on a multiplayer server must use this extension. Its network revision
  ends in `-c240rgb`; older 240-company and ordinary JGRPP clients are rejected before joining.
* Saved server password invalidation masks and live password updates cover all
  240 companies. Server admin statistics continue to send one packet per company.

The company performance league table and script league tables have clipped,
resizable views with scrollbars, including access to the final company. Script
headers and footers scroll with their table and links retain their correct row.
Graph legends, performance and script company selectors, the link-graph company
selector, local-authority ratings and the minimap company legend have scrollbars.
The AI configuration and network client lists already have scrolling lists.
AI performance tracking covers every company, and its display initially shows
at most twenty rows with scrolling for the remaining measurements.

## Verification

Build the game and test target with the usual CMake configuration:

```text
cmake --build build-240 --config Debug --target openttd openttd_test --parallel
build-240\Debug\openttd_test.exe "[company240]"
build-240\Debug\openttd_test.exe
```

The `company240` tests cover every company mask bit, the 30-byte network mask,
independent tile/road/tram owners, legacy packed owners, cargo-monitor collisions,
vehicle-list identities, AI performance tracking and scrolling to the last company.
They also check that changing an owner preserves adjacent road flags and water classes.
RGB tests cover parsing, network command serialization, black, white, independent
primary/secondary ramps, exact opaque pixels, alpha blending, clipped sprites and
palette animation across the available 32-bit blitters. Sharing tests exercise
every company as both payer and recipient, including fractional station fees,
rail fees, yearly cost/revenue, quarterly income/expenses, vehicle profit, debt
limits, public ownership and payments within the same company.

Validated on Windows with MSVC:

* The game and test executable build with assertions and compression support.
* All 205 test cases pass, with 72,675 assertions.
* A local server creates and runs 240 AI companies. Admin polls return all 240
  company IDs for company information, economy and vehicle/station statistics.
* Saving and restarting that server retains all 240 companies.
* A Windows client loads the old version-4 title screen, downloads the 240-company
  game and remains connected as a spectator.
* The existing station-list regression save loads, converts and reloads with both
  original companies intact.
* RGB primary and secondary values for company numbers 1, 16, 128 and 240 survive
  server save/reload, including pure black and white. All 240 companies remain present.
* A client joins that RGB save with animated 32-bit and OpenGL graphics. The league
  scrollbar reaches rank 240, and entering a hex value through company 240's colour
  dialog updates its swatch and displayed RGB value.
* Rendering regressions check actual graph-line pixels and animation-buffer entries
  in 40-bit mode, company label frames, title-bar fills and RGB font glyphs. These
  cover non-palette colours, black and white, and run alongside 32-bit renderers.
* An OpenGL client renders matching RGB graph dots and connecting lines, owned
  title bars, opaque station labels, and transparent station/sign text.
