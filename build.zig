const std = @import("std");

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const multicore = b.option(bool, "multicore", "Enables multicore for libff") orelse false;

    const ff_dep = b.dependency("ff", .{});
    const gmp_dep = b.dependency("gmp", .{
        .target = target,
        .optimize = optimize,
    });
    const sodium_dep = b.dependency("sodium", .{
        .target = target,
        .optimize = optimize,
        .shared = false,
    });

    const ff = b.addStaticLibrary(.{
        .name = "ff",
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(ff);

    ff.linkLibCpp();
    ff.addIncludePath(ff_dep.path("."));
    ff.linkLibrary(sodium_dep.artifact("sodium"));
    ff.linkLibrary(gmp_dep.artifact("gmp"));

    ff.addCSourceFiles(.{
        .root = ff_dep.path("libff"),
        .files = &.{
            "algebra/curves/alt_bn128/alt_bn128_fields.cpp",
            "algebra/curves/alt_bn128/alt_bn128_g1.cpp",
            "algebra/curves/alt_bn128/alt_bn128_g2.cpp",
            "algebra/curves/alt_bn128/alt_bn128_init.cpp",
            "algebra/curves/alt_bn128/alt_bn128_pairing.cpp",
            "algebra/curves/alt_bn128/alt_bn128_pp.cpp",
            "common/double.cpp",
            "common/profiling.cpp",
            "common/utils.cpp",
        },
        .flags = &.{
            "-DCURVE_ALT_BN128",
            "-DNO_PROCPS",
            "-DNO_PT_COMPRESSION",
            if (multicore) "-DMULTICORE" else "",
        },
    });
    ff.installHeadersDirectory(
        ff_dep.path("libff"),
        "libff/",
        .{ .include_extensions = &.{ ".hpp", ".tcc" } },
    );
    ff.addCSourceFile(.{ .file = b.path("src/binding.cxx") });

    const binding_translate = b.addTranslateC(.{
        .target = target,
        .optimize = optimize,
        .root_source_file = b.path("src/binding.h"),
    });

    const header_mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .root_source_file = binding_translate.getOutput(),
    });
    header_mod.linkLibrary(ff);

    const ff_mod = b.addModule("ff", .{
        .target = target,
        .optimize = optimize,
        .root_source_file = b.path("src/root.zig"),
    });
    ff_mod.addImport("ff", header_mod);

    const test_exe = b.addTest(.{
        .target = target,
        .optimize = optimize,
        .root_source_file = b.path("src/test.zig"),
    });

    test_exe.root_module.addImport("ff", ff_mod);
    const run_test_exe = b.step("test", "Runs the test_exe");
    run_test_exe.dependOn(&b.addRunArtifact(test_exe).step);
}
