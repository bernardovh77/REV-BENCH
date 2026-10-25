import unittest
import numpy as np
from gas_graph_reference import SimpleWave

class SimpleWaveReferenceTests(unittest.TestCase):
    def setUp(self):
        # Independent calorically perfect control; sensible zero at298.15K.
        self.R=287.;self.cv=717.5;self.gamma=1.4
        self.eos=dict(R=self.R,lo=[self.cv,0,0,0,0],hi=[self.cv,0,0,0,0],k_lo=-298.15*self.cv,k_hi=-298.15*self.cv)

    def test_perfect_gas_isentropic_integrals(self):
        wave=SimpleWave(self.eos,300,amplitude=.01)
        p,T,u=wave.initial(np.array([.1,.11,.13]))
        expected=300*(p/101325)**((self.gamma-1)/self.gamma)
        np.testing.assert_allclose(T,expected,rtol=1e-13)
        np.testing.assert_allclose(u,2*(np.sqrt(self.gamma*self.R*T)-wave.c0)/(self.gamma-1),atol=1e-11)

    def test_zero_time_and_uniform_average(self):
        wave=SimpleWave(self.eos,300,amplitude=0)
        x=np.linspace(0,1,20);dx=np.full_like(x,.05)
        np.testing.assert_allclose(wave.pressure_of_average(x,dx,.002),101325,rtol=1e-13)
        rho,u,E=wave.state(x,0)
        np.testing.assert_allclose(rho,101325/(self.R*300),rtol=1e-14)
        np.testing.assert_array_equal(u,np.zeros_like(x))
        np.testing.assert_allclose(E/rho,self.cv*(300-298.15),atol=1e-10)

    def test_characteristic_foot_consistency(self):
        wave=SimpleWave(self.eos,300,amplitude=.001)
        foot=np.array([.08,.10,.12]);p,T,u=wave.initial(foot);time=.001
        x=foot+(u+wave.sound(T))*time
        rho,velocity,E=wave.state(x,time)
        np.testing.assert_allclose(velocity,u,atol=1e-10)
        np.testing.assert_allclose(rho,p/(self.R*T),rtol=1e-12)
