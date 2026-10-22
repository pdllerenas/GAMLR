fn main() {
    let owds = vec![60.6031,61.9581,70.8311,89.6331,99.9161];

    // Pass 'owds' by value to yield f64, and 'None' for the seed
    let estimated_offset = gamlr::estimate(owds, None);

    println!("Estimated offset based on OWD: {}", estimated_offset);
}
