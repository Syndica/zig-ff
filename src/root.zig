const std = @import("std");
const ff = @import("ff");

pub const G1 = struct {
    point: ff.bn254_point_g1_t,
    is_zero: bool,

    const zero: G1 = .{ .point = .{ .v = .{0} ** 64 }, .is_zero = true };

    pub fn fromBytes(bytes: [64]u8) !G1 {
        // Special case, if all bytes are zero, return point at infinity
        if (std.mem.allEqual(u8, &bytes, 0)) return zero;
        const p: ff.bn254_point_g1_t = .{ .v = bytes };
        if (ff.bn254_g1_check(&p) != 1) return error.NonCanonical;
        return .{ .point = p, .is_zero = ff.bn254_g1_is_zero(&p) };
    }

    pub fn toBytes(p: G1) [64]u8 {
        if (p.is_zero) return .{0} ** 64;
        return p.point.v;
    }

    pub fn format(
        p: G1,
        comptime _: []const u8,
        _: std.fmt.FormatOptions,
        writer: anytype,
    ) !void {
        const result = ff.bn254_g1_print(&p.point);
        try writer.writeAll(std.mem.span(result));
    }
};

pub const G2 = struct {
    point: ff.bn254_point_g2_t,
    is_zero: bool,

    const zero: G2 = .{ .point = .{ .v = .{0} ** 128 }, .is_zero = true };

    pub fn fromBytes(bytes: [128]u8) !G2 {
        // Special case, if all bytes are zero, return point at infinity
        if (std.mem.allEqual(u8, &bytes, 0)) return zero;
        const p: ff.bn254_point_g2_t = .{ .v = bytes };
        if (ff.bn254_g2_check(&p) != 1) return error.NonCanonical;
        return .{ .point = p, .is_zero = ff.bn254_g2_is_zero(&p) };
    }

    pub fn format(
        p: G2,
        comptime _: []const u8,
        _: std.fmt.FormatOptions,
        writer: anytype,
    ) !void {
        const result = ff.bn254_g2_print(&p.point);
        try writer.writeAll(std.mem.span(result));
    }
};

pub const Scalar = struct {
    bytes: [32]u8,
};

pub fn affineAdd(a: G1, b: G1) G1 {
    if (a.is_zero) return b;
    if (b.is_zero) return a;

    var result: ff.bn254_point_g1_t = undefined;
    ff.bn254_g1_add(&a.point, &b.point, &result);
    return .{
        .point = result,
        .is_zero = ff.bn254_g1_is_zero(&result),
    };
}

pub fn scalarMul(a: G1, s: Scalar) G1 {
    const b: ff.bn254_bigint_t = .{ .v = s.bytes };
    var result: ff.bn254_point_g1_t = undefined;
    ff.bn254_g1_mul(&a.point, &b, &result);
    return .{
        .point = result,
        .is_zero = ff.bn254_g1_is_zero(&result),
    };
}

pub fn pairIsOne(p: []const G1, q: []const G2) bool {
    std.debug.assert(p.len == q.len and p.len <= 16);

    var p_p: std.BoundedArray(ff.bn254_point_g1_t, 16) = .{};
    var q_p: std.BoundedArray(ff.bn254_point_g2_t, 16) = .{};

    for (p, q) |a, b| {
        p_p.appendAssumeCapacity(a.point);
        q_p.appendAssumeCapacity(b.point);
    }

    return ff.bn254_pairing(
        p_p.constSlice().ptr,
        q_p.constSlice().ptr,
        @intCast(p.len),
    );
}
