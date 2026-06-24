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
== Callback Style Animation

#slide(
  repeat: 3,
  self => [
    #let (uncover, only, alternatives) = utils.methods(self)

    At subslide #self.subslide, we can

    use #uncover("2-")[`#uncover` function] for reserving space,

    use #only("2-")[`#only` function] for not reserving space,

    #alternatives[call `#only` multiple times \u{2717}][use `#alternatives` function #sym.checkmark] for choosing one of the alternatives.
  ],
)


== Math Equation Animation

Equation with `pause`:

$
  f(x) & = pause x^2 + 2x + 1 \
       & = pause (x + 1)^2 \
$

#meanwhile

Here, #pause we have the expression of $f(x)$.

#pause

By factorizing, we can obtain this result.


== CeTZ Animation

CeTZ Animation in Touying:

#cetz-canvas({
  import cetz.draw: *

  rect((0, 0), (5, 5))

  (pause,)

  rect((0, 0), (1, 1))
  rect((1, 1), (2, 2))
  rect((2, 2), (3, 3))

  (pause,)

  line((0, 0), (2.5, 2.5), name: "line")
})


== Fletcher Animation

Fletcher Animation in Touying:

#fletcher-diagram(
  node-stroke: .1em,
  node-fill: gradient.radial(blue.lighten(80%), blue, center: (30%, 20%), radius: 80%),
  spacing: 4em,
  edge((-1, 0), "r", "-|>", `open(path)`, label-pos: 0, label-side: center),
  node((0, 0), `reading`, radius: 2em),
  edge((0, 0), (0, 0), `read()`, "--|>", bend: 130deg),
  pause,
  edge(`read()`, "-|>"),
  node((1, 0), `eof`, radius: 2em),
  pause,
  edge(`close()`, "-|>"),
  node((2, 0), `closed`, radius: 2em, extrude: (-2.5, 0)),
  edge((0, 0), (2, 0), `close()`, "-|>", bend: -40deg),
)


= Theorems

== Prime numbers

#definition[
  A natural number is called a #highlight[_prime number_] if it is greater
  than 1 and cannot be written as the product of two smaller natural numbers.
]
#example[
  The numbers $2$, $3$, and $17$ are prime.
  @cor_largest_prime shows that this list is not exhaustive!
]

#theorem(title: "Euclid")[
  There are infinitely many primes.
]
#pagebreak(weak: true)
#proof[
  Suppose to the contrary that $p_1, p_2, dots, p_n$ is a finite enumeration
  of all primes. Set $P = p_1 p_2 dots p_n$. Since $P + 1$ is not in our list,
  it cannot be prime. Thus, some prime factor $p_j$ divides $P + 1$. Since
  $p_j$ also divides $P$, it must divide the difference $(P + 1) - P = 1$, a
  contradiction.
]

#corollary[
  There is no largest prime number.
] <cor_largest_prime>
#corollary[
  There are infinitely many composite numbers.
]

#theorem[
  There are arbitrarily long stretches of composite numbers.
]

#proof[
  For any $n > 2$, consider $ n! + 2, quad n! + 3, quad ..., quad n! + n $
]


= Others

== Multiple columns

#cols[
  First column.
][
  Second column.
]

== Multiple columns with equal height blocks

#cols(columns: (1fr, 1fr), gutter: 1em, lazy-layout: true)[
  #emph-block[
    First column with equal height: #lorem(10)
    #lazy-v(1fr)
  ]
][
  #emph-block[
    Second column with equal height: : #lorem(15)
    #lazy-v(1fr)
  ]
]


== Multiple Pages

#lorem(200)


#show: appendix

= Appendix

== Appendix

Please pay attention to the current slide number.
