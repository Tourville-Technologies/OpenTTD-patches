# More Companies 1.0.4

More Companies 1.0.4 fixes two displays that used the company's old palette colour after a custom RGB colour had been selected. It is based on JGRPP 0.73.5 and includes all 1.0.3 cargo-aging and payment protections.

## Ownership map

The main viewport's ownership view, selected with Page Up/Page Down while zoomed out to map mode, now retains the primary RGB colour through flat terrain, slope shading, station highlights, and ownership-coloured bridges and tunnels. Changing the RGB colour refreshes cached map pixels immediately.

Ownership colours retain the map's normal lighter shade and slope/structure shading. Standard palette colours and 8-bit fallback indices keep their existing behavior.

## CEO background

The background gradient behind the company CEO now uses the primary RGB colour. The company window, face editor, takeover dialog, and company error portraits share this rendering path. Face features and clothing are not recoloured.

## Validation

* Full game suite: 72,982 assertions in 211 test cases passed.
* The ownership regression test covers company 240, both reported brown colours, black, white, terrain shading, station pixels, standard palette selection, and 8-bit fallback output.
* Native UI checks reproduce both mismatches in the released 1.0.3 executable and verify the fixes with 32bpp-anim and OpenGL 40bpp-anim.
* Each UI check builds 16 owned rail pieces and changes the primary RGB colour from #744821 to #0088CC without reloading. All 16 ownership pixels update to the selected colour. The portrait gradient updates while face features remain identical.

Download the Windows 64-bit ZIP, extract it, and run its openttd.exe. Existing scenario settings remain available, including the optional minimum cargo fare introduced in 1.0.3. Use the same version for all multiplayer participants.
