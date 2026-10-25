"""Independent exact-reference controls; no numerical-candidate fitting."""
import unittest
from check_mixture import exact_riemann

class RiemannReferenceTests(unittest.TestCase):
    def test_uniform_and_mirror(self):
        state=(1.,0.,1.)
        for x in (-3.,-.1,0.,.1,3.):
            self.assertEqual(exact_riemann(x,1,state,state,1.4),state)
        left,right=(1.,0.,1.),(.125,0.,.1)
        for x in (-1.,-.2,0.,.2,1.):
            a=exact_riemann(x,1,left,right,1.4)
            b=exact_riemann(-x,1,right,left,1.4)
            for av,bv in zip(a,(b[0],-b[1],b[2])): self.assertAlmostEqual(av,bv,places=10)

    def test_textbook_sod_star(self):
        rho,u,p=exact_riemann(0,1,(1,0,1),(.125,0,.1),1.4)
        self.assertAlmostEqual(rho,.426319428,places=8)
        self.assertAlmostEqual(u,.927452620,places=8)
        self.assertAlmostEqual(p,.303130178,places=8)

    def test_expansion_is_symmetric_and_cools(self):
        a=exact_riemann(0,1,(1,-.5,1),(1,.5,1),1.4)
        self.assertAlmostEqual(a[1],0,places=12)
        self.assertLess(a[0],1);self.assertLess(a[2],1)
