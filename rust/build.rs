fn main() {
    println!("cargo:rustc-link-lib=static=ganglion");
    println!("cargo:rustc-flags=-l stdc++ -l dl -l rt -l z"); // something else for mac
    println!("cargo:rustc-link-search={}", "../build");
}
