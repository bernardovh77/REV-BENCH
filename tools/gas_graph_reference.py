"""Independent fixed-composition, variable-cp simple-wave reference for045.

Uses only declared EOS coefficients, quadrature and continuum characteristics;
never reads fitted travel/phase from a candidate trajectory. Not real-engine
calibration or chemistry validation.
"""
import numpy as np

class SimpleWave:
    def __init__(self,eos,temperature,pressure=101325.,center=.1,width=.035,amplitude=.001,frequency=0.):
        self.eos=eos;self.T0=temperature;self.p0=pressure;self.center=center;self.width=width
        self.amplitude=amplitude;self.frequency=frequency
        self.nodes,self.weights=np.polynomial.legendre.leggauss(24)
        self.c0=float(self.sound(np.array(temperature)))

    def cv(self,T):
        t=np.asarray(T)/1000
        return np.where(np.asarray(T)<=1000,np.polynomial.polynomial.polyval(t,self.eos['lo']),
                        np.polynomial.polynomial.polyval(t,self.eos['hi']))

    def sound(self,T):return np.sqrt((1+self.eos['R']/self.cv(T))*self.eos['R']*T)

    def energy(self,T):
        t=np.asarray(T)/1000
        lo=np.array(self.eos['lo'])/np.arange(1,6);hi=np.array(self.eos['hi'])/np.arange(1,6)
        return np.where(np.asarray(T)<=1000,self.eos['k_lo']+T*np.polynomial.polynomial.polyval(t,lo),
                        self.eos['k_hi']+T*np.polynomial.polynomial.polyval(t,hi))

    def integral(self,T,kind):
        T=np.asarray(T);samples=self.T0+(T[...,None]-self.T0)*(self.nodes+1)/2
        cp=self.cv(samples)+self.eos['R']
        value=cp/samples if kind=='entropy' else cp/self.sound(samples)
        return (T-self.T0)/2*np.sum(value*self.weights,axis=-1)

    def initial(self,x):
        distance=(x-self.center)/self.width
        dp=np.where(abs(distance)<=6,self.amplitude*np.exp(-distance**2)*
                    np.cos(2*np.pi*self.frequency*(x-self.center)/self.c0),0)
        p=self.p0*(1+dp);target=self.eos['R']*np.log1p(dp)
        T=self.T0*np.exp(target/(self.cv(np.array(self.T0))+self.eos['R']))
        for _ in range(8):T-=(self.integral(T,'entropy')-target)*T/(self.cv(T)+self.eos['R'])
        return p,T,self.integral(T,'velocity')

    def state(self,x,time):
        x=np.asarray(x);foot=x-self.c0*time
        for _ in range(24):
            _,T,u=self.initial(foot);next_foot=x-(u+self.sound(T))*time
            error=np.max(abs(next_foot-foot));foot=next_foot
            if error<2e-14:break
        p,T,u=self.initial(foot)
        rho=p/(self.eos['R']*T)
        return rho,u,rho*(self.energy(T)+.5*u*u)

    def pressure_of_average(self,x,dx,time):
        nodes,weights=np.polynomial.legendre.leggauss(8)
        at=np.asarray(x)[:,None]+np.asarray(dx)[:,None]*nodes/2
        local_rho,local_u,local_E=self.state(at,time)
        # Each conserved quantity is averaged before nonlinear EOS decoding.
        rho=np.sum(local_rho*weights/2,axis=1)
        momentum=np.sum(local_rho*local_u*weights/2,axis=1);energy=np.sum(local_E*weights/2,axis=1)
        internal=energy/rho-.5*(momentum/rho)**2
        T=np.full_like(rho,self.T0)
        for _ in range(10):T-=(self.energy(T)-internal)/self.cv(T)
        return rho*self.eos['R']*T
