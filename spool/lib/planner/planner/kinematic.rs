use crate::platform;
use core::ops::{Add, Sub};
use fixed_sqrt::FixedSqrt;

use fixed::{
    traits::{Fixed, FixedUnsigned, ToFixed},
    types::{I20F12, U20F12},
};

#[repr(u32)]
#[derive(Debug, PartialEq, Eq)]
pub enum KinematicKind {
    Undef = 0,
    I3,
    CoreXY,
}

impl KinematicKind {
    pub fn plan<I>(&self, cart: &[I], out: &mut [I])
    where
        I: Add<Output = I> + Sub<Output = I> + Copy,
    {
        match self {
            KinematicKind::Undef => panic!(),
            KinematicKind::I3 => {
                out.copy_from_slice(cart);
            }
            KinematicKind::CoreXY => {
                assert!(cart.len() >= 2, "AXIS insufficient");
                out[0] = cart[0] + cart[1];
                out[1] = cart[0] - cart[1];
                out[2..].copy_from_slice(&cart[2..]);
            }
        }
    }

    pub fn move_len<I>(&self, cart: &[I], extruder: &[I]) -> I::Unsigned
    where
        I: Fixed,
        I::Unsigned: FixedUnsigned,
    {
        let minimum_move_threshold = I::Unsigned::from_num(0);

        match self {
            KinematicKind::Undef => panic!(),
            KinematicKind::I3 | KinematicKind::CoreXY => {
                let cart_or_extruder = if cart
                    .iter()
                    .any(|x| x.abs_diff(0.to_fixed::<I>()) > minimum_move_threshold)
                {
                    cart
                } else {
                    extruder
                };
                let squared_sum = cart_or_extruder
                    .iter()
                    .map(|x| x.to_num::<f32>())
                    .fold(0f32, |acc, elem| elem * elem + acc);
                platform::platform_sqrtf(squared_sum).to_fixed::<I::Unsigned>()
            }
        }
    }
}
