#import "@preview/touying:0.7.4": *
#import themes.university: *
#import "@preview/cetz:0.5.2"
#import "@preview/fletcher:0.5.8" as fletcher: edge, node
#import "@preview/numbly:0.1.0": numbly
#import "@preview/theorion:0.6.0": *
#import cosmos.clouds: *
#show: show-theorion

// cetz and fletcher bindings for touying
#let cetz-canvas = touying-reducer.with(reduce: cetz.canvas, cover: cetz.draw.hide.with(bounds: true))
#let fletcher-diagram = touying-reducer.with(reduce: fletcher.diagram, cover: fletcher.hide)

#show: university-theme.with(
  aspect-ratio: "16-9",
  // align: horizon,
  // config-common(handout: true),
  config-common(frozen-counters: (theorem-counter,)), // freeze theorem counter for animation
  config-info(
    title: [SPRUCE],
    subtitle: [Spread PaiR Unused Capacity Estimate],
    author: [Pedro D. Llerenas],
    date: datetime.today(),
    institution: [CIMAT],
  ),
)

#set heading(numbering: numbly("{1}.", default: "1.1"))

#title-slide()

= Definitions

#figure(image("/assets/image.png"), caption: [Difference between tight link and narrow link.])
#figure(
  image("/assets/image-3.png"),
  caption: [Probe Rate Model (PRM). Self-induced congestion: sends probe traffic at increased rates until delays spike.],
)
#figure(
  image("/assets/image-2.png"),
  caption: [Probe Gap Model (PGM). Dispersion tracking: sends a pair of packets with a specific time gap. Measures how much cross-traffic squeezes between them.],
)

= Pathload
== Pathload
Uses PRM. Binary search on the congestion point.

*Convergence*: 12-30 s.
*Intrusiveness*: High. Creates network congestion until convergence.
*Estimate*: Tends to overestimate.

= IGI
== IGI
Intial Gap Increasing uses PGM to find the inflection point where the input gap matches the output gap.
It works in quiet networks, fails in congested networks, and it overestimates.

= SPRUCE
== SPRUCE
Calculates the exact bytes of cross-traffic. The available bandwidth is given by
$
  A = C (1- (Delta_("out") - Delta_"in")/Delta_"in")
$
where $C$ is the bottleneck capacity.

We use a Poisson sampling process between each packet pair.
= Comparison
== Comparison
#figure(image("/assets/image-4.png"), caption: [100 Mb/s LCS-MIT])
#figure(image("/assets/image-5.png"),caption: [100 Mb/s UC Berkley - MIT])