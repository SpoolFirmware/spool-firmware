load("@rules_rust//rust:repositories.bzl", "rust_repository_set")

def register_rust_toolchains(host_triples, target_triples, version):
    for host in host_triples:
        name = "rust-" + version + "-" + host 
        rust_repository_set(
            name=name,
            exec_triple=host,
            versions=[version],
            extra_target_triples=target_triples,
        )