"""Independent049 geometry and Helmholtz controls, before native promotion."""
import copy,math,unittest
import numpy as np
from junction_mesh import fixture,helmholtz,native_input,compact_transfer

class JunctionMeshTests(unittest.TestCase):
    def test_h_targeted_refinement_keeps_geometry_and_port_identity(self):
        base=fixture('h',32);fine=fixture('h',32,h_transport_n=48)
        self.assertEqual(len(fine['cells']),8192)
        self.assertEqual(base['ports'],fine['ports'])
        self.assertEqual(base['source_outlet_pairs'],fine['source_outlet_pairs'])
        self.assertAlmostEqual(base['volume_m3'],fine['volume_m3'],places=14)
        for region in range(7):
            for cold in [False,True]:
                def volume(m):return math.fsum(c['volume_m3']*(c['left_volume_fraction'] if cold else 1) for c in m['cells'] if c['region']==region)
                self.assertAlmostEqual(volume(base),volume(fine),places=14)

    def test_fine_y_half_plane_partition_stays_geometric(self):
        volumes=[]
        for n in [8,16,24,32]:
            mesh=fixture('y',n);fractions=[c['left_volume_fraction'] for c in mesh['cells']]
            self.assertTrue(all(0<=a<=1 for a in fractions))
            volumes.append(math.fsum(c['volume_m3']*c['left_volume_fraction'] for c in mesh['cells']))
        np.testing.assert_allclose(volumes,volumes[0],rtol=1e-12,atol=1e-18)

    def test_compact_physical_stubs_are_passive_and_reciprocal(self):
        for kind in ['area','y','x','h']:
            mesh=fixture(kind,8);areas=np.array([p['area_m2'] for p in mesh['ports']]);weights=np.diag(np.sqrt(areas))
            for finite in [False,True]:
                matrices=compact_transfer(mesh,[1,200,1500,4000],580.1001671698209,finite)
                for matrix in matrices:
                    normalized=weights@matrix@np.linalg.inv(weights)
                    np.testing.assert_allclose(normalized.conj().T@normalized,np.eye(len(areas)),atol=2e-12)
                    np.testing.assert_allclose(normalized,normalized.T,atol=2e-12)

    def test_exact_volume_port_area_and_oriented_cell_surfaces(self):
        W=.04;D=.04
        expected={'straight':3*W**2*D,'area':2.5*W**2*D,'y':(math.sqrt(3)/4+2.25)*W**2*D,'x':5*W**2*D,'h':7*W**2*D}
        for kind in expected:
            m=fixture(kind,8)
            self.assertAlmostEqual(m['volume_m3'],expected[kind],places=14)
            closure=np.zeros((len(m['cells']),3));volumes=np.zeros(len(m['cells']))
            for f in m['faces']:
                n=np.array(f['normal']);center=np.array(f['center_m']);A=f['area_m2']
                for i,sign in [(f['a'],1),(f['b'],-1)]:
                    if i<0:continue
                    closure[i]+=sign*A*n;volumes[i]+=sign*A*np.dot(n,center-np.array(m['cells'][i]['center_m']))/3
            self.assertLess(np.max(abs(closure)),1e-15)
            np.testing.assert_allclose(volumes,[c['volume_m3'] for c in m['cells']],rtol=1e-11,atol=1e-18)
            self.assertEqual(len(m['source_outlet_pairs']),len(m['ports'])**2)

    def test_exact_thermal_partition_and_no_geometry_mutation(self):
        for kind in ['y','x','h','area','straight']:
            a=fixture(kind,8);b=fixture(kind,16)
            cold=lambda m:sum(c['volume_m3']*c['left_volume_fraction'] for c in m['cells'])
            self.assertAlmostEqual(cold(a),cold(b),places=14)
            before=copy.deepcopy(a);native_input(a);self.assertEqual(a,before)
            for c in a['cells']:self.assertTrue(0<=c['left_volume_fraction']<=1)

    def test_independent_helmholtz_matches_analytic_duct(self):
        c=347.24387326840287;frequencies=[200,1000,2000];m=fixture('straight',24);S=helmholtz(m,frequencies,c)
        target=np.exp(-2j*np.pi*np.array(frequencies)*.12/c)
        self.assertLess(np.max(abs(S[:,0,1]-target)),.01)
        self.assertLess(np.max(abs(S[:,1,0]-target)),.01)
        self.assertLess(np.max(abs(S[:,0,0])),.01)

    def test_independent_helmholtz_reciprocity_and_compact_limit(self):
        m=fixture('y',8);areas=np.array([p['area_m2'] for p in m['ports']]);S=helmholtz(m,[1,200],347.24387326840287)
        normalized=S*np.sqrt(areas[None,:,None]/areas[None,None,:])
        np.testing.assert_allclose(normalized,np.swapaxes(normalized,1,2),atol=1e-10,rtol=1e-10)
        compact=2*areas[None,:]/areas.sum()-np.eye(3)
        self.assertLess(np.max(abs(S[0]-compact)),.02)

if __name__=='__main__':unittest.main()
