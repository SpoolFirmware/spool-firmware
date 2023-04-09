use core::ops::{Sub, Add};

use fixed::types::I20F12;

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
        I: Add<Output=I> + Sub<Output=I> + Copy,
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
}
