# Screen integration review — October 28, 2026

This branch consolidates browser fixes from the screen PR review on top of
PR #25, reconciled with main at `99a39c1`. It is a draft integration, not a
firmware promotion or live deployment.

## Final behavior

- The page notices presentation-surface changes even when firmware pixels do
  not change. Context-loss fallback paints the static screen on the next update.
- Presenter deduplication checks context health first. A throttled final frame
  explicitly returns false and remains pending until displayed.
- Exact pixels, ImageData reuse, a 15-Hz compatible ceiling, Canvas defaults,
  physical focus and opt-in/off Audio priority remain. The older 45-ms polling
  experiment is not enabled or copied into the active companion.
- Lightweight-display reconnection/hidden drawing improvements from #22 are
  included separately from the graphics page.
- Native-focus packaging retains its own page/controller. Legacy packaging
  includes the router used for already-installed candidate firmware.
- Site publishing preserves main's installed-device routes and the optional
  release hub. The existing PR #25 URL is staged as a full pinned bundle with
  its new presenter and recent-rate label. Explicit browser trials are excluded
  from automatic renderer routing, so they cannot replace the default page for
  all devices sharing a renderer ID.
- Main's alpha warnings, installer link, Paleotone artwork, owner attribution
  policy and parked VST plans are retained. Riders stays retired.

## Validation

The focused integration workflow runs presenter/page-cache regressions, exact
colors/cadence, synthetic workloads, lightweight reconnection, routing, physical
focus and packaging tests. It executes the actual production preservation step
against the published tree without publishing, validates installed routes and
the pinned full trial, and tests actual Canvas/PNG presentation in Chrome.

Local tests cover both layouts and both physical focus sides, static context
loss, compatible-mode deferral and returning to Canvas. Existing hardware
normal-play/focus acceptance remains complete; do not request it again without
a new symptom. The earlier clean A/B still does not prove an Audio priority
benefit. No new hardware timing or Android acceptance is claimed.

The staged artifact contains retained published pages and an updated browser
trial. It does not contain a newly built/promoted integration firmware. New
firmware changes still require the separate target/host/worker acceptance gates.

## Relationship to the earlier PRs

#24 depends on #23; #25 belongs to the ROAR-worker firmware stack instead.
This branch imports selected browser changes without merging #23/#24's older
page or polling policy. No older PR was closed or merged automatically.
Review this PR relative to #25 first. Main promotion and firmware release
remain separate decisions.
