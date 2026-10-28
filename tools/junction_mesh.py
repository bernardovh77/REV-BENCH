"""049 physical prism meshes and independently assembled Helmholtz references.

Research fixtures only; no claim these are surveyed catalog exhaust interiors.
One SI polygon definition supplies both methods and semantic port identities.
"""
import json,math
from pathlib import Path
import numpy as np

class Mesh:
    def __init__(self,depth):
        self.depth=depth;self.vertices=[];self.lookup={};self.polygons=[];self.regions=[];self.ports=[]

    def point(self,p):
        key=tuple(round(float(v),14) for v in p)
        if key not in self.lookup:self.lookup[key]=len(self.vertices);self.vertices.append(tuple(map(float,p)))
        index=self.lookup[key]
        if math.dist(self.vertices[index],p)>1e-12:raise ValueError('Distinct vertices cannot be merged')
        return index

    def polygon(self,points,region):
        ids=[self.point(p) for p in points];p=np.array([self.vertices[i] for i in ids])
        following=np.roll(p,-1,axis=0);cross=(p[:,0]*following[:,1]-p[:,1]*following[:,0]).sum()
        if cross<0:ids.reverse()
        if abs(cross)<1e-18:raise ValueError('Degenerate cell')
        self.polygons.append(ids);self.regions.append(region)

    def rectangle(self,origin,axis,tangent,length,width,nx,ny,region):
        origin=np.array(origin);axis=np.array(axis);tangent=np.array(tangent)
        def p(i,j):return origin+i*length/nx*axis+j*width/ny*tangent
        for i in range(nx):
            for j in range(ny):self.polygon([p(i,j),p(i+1,j),p(i+1,j+1),p(i,j+1)],region)

    def stub(self,a,b,length,n,region,identity,fraction=1,axial_n=None):
        a=np.array(a);b=np.array(b);W=float(np.linalg.norm(b-a));t=(b-a)/W;axis=np.array([t[1],-t[0]])
        count=round(n*fraction)
        if abs(count-n*fraction)>1e-10 or (n-count)%2:raise ValueError('Port patch must conform to center mesh')
        origin=a+(n-count)/2*W/n*t;width=count*W/n
        self.rectangle(origin,axis,t,length,width,axial_n if axial_n is not None else max(2,round(n*length/W)),count,region)
        self.ports.append(dict(id=identity,center=(origin+length*axis+.5*width*t).tolist(),normal=axis.tolist(),width=width,area_m2=width*self.depth))

    def prepared(self,identity):
        vertices=np.array(self.vertices);cells=[];faces=[];edges={}
        for ci,ids in enumerate(self.polygons):
            p=vertices[ids];nextp=np.roll(p,-1,axis=0);cross=p[:,0]*nextp[:,1]-nextp[:,0]*p[:,1]
            area=float(cross.sum()/2);center=((p+nextp)*cross[:,None]).sum(axis=0)/(6*area)
            if p[:,0].max()<=0:left=1.
            elif p[:,0].min()>=0:left=0.
            else:
                def half_area(sign):
                    clipped=[]
                    for u,v in zip(p,nextp):
                        if sign*u[0]<=0:clipped.append(u)
                        if (sign*u[0]<0)!=(sign*v[0]<0):clipped.append(u+(v-u)*(-u[0])/(v[0]-u[0]))
                    if len(clipped)<3:return 0.
                    cp=np.array(clipped);cp=cp-cp[0];cn=np.roll(cp,-1,axis=0)
                    return abs(math.fsum(float(x) for x in cp[:,0]*cn[:,1]-cn[:,0]*cp[:,1]))/2
                # Two nonnegative geometric half-volumes avoid cancellation at
                # near-coplanar vertices. Dividing one shoelace area by another
                # could produce1+epsilon and an invalid native initial state.
                # This partitions geometry; no simulated inventory is clipped.
                va,vb=half_area(1),half_area(-1);left=va/(va+vb)
            cells.append(dict(volume_m3=area*self.depth,center_m=[*center,0.],region=self.regions[ci],left_volume_fraction=left))
            for j,a in enumerate(ids):
                b=ids[(j+1)%len(ids)];key=tuple(sorted((a,b)))
                if key in edges:
                    f=faces[edges[key]]
                    if f['b']!=-1:raise ValueError('Non-manifold mesh')
                    f['b']=ci
                else:
                    d=vertices[b]-vertices[a];L=float(np.linalg.norm(d));mid=(vertices[a]+vertices[b])/2
                    edges[key]=len(faces);faces.append(dict(a=ci,b=-1,port=-1,area_m2=L*self.depth,normal=[d[1]/L,-d[0]/L,0.],center_m=[*mid,0.],edge_vertices=[a,b]))
            for sign in (-1,1):faces.append(dict(a=ci,b=-1,port=-1,area_m2=area,normal=[0.,0.,float(sign)],center_m=[*center,sign*self.depth/2]))
        areas=np.zeros(len(self.ports));counts=np.zeros(len(self.ports),dtype=int)
        for f in faces:
            if f['b']>=0 or abs(f['normal'][2])>.5:continue
            p=np.array(f['center_m'][:2]);normal=np.array(f['normal'][:2])
            for i,port in enumerate(self.ports):
                n=np.array(port['normal']);delta=p-np.array(port['center']);t=np.array([-n[1],n[0]])
                if np.dot(n,normal)>1-1e-12 and abs(np.dot(n,delta))<1e-10 and abs(np.dot(t,delta))<port['width']/2+1e-12:
                    if f['port']!=-1:raise ValueError('Overlapping source/outlet ports')
                    f['port']=i;areas[i]+=f['area_m2'];counts[i]+=1
        for i,p in enumerate(self.ports):
            if not counts[i] or not math.isclose(areas[i],p['area_m2'],rel_tol=1e-10):raise ValueError('Incomplete port binding')
        return dict(schema_version=1,kind='finite-junction-research-fixture',id=identity,units='SI',dimension_basis='explicit-synthetic-geometry',depth_m=self.depth,
                    vertices_m=vertices.tolist(),polygons=self.polygons,regions=self.regions,cells=cells,faces=faces,ports=self.ports,
                    volume_m3=sum(c['volume_m3'] for c in cells),source_outlet_pairs=[dict(source=a['id'],outlet=b['id'],connected=True) for a in self.ports for b in self.ports])

def fixture(kind,n=16,width=.04,depth=.04,stub_ratio=1,h_transport_n=None):
    if kind not in ['y','x','h','area','straight'] or n<2 or n>64:raise ValueError('Unsupported fixture')
    if h_transport_n is not None and (kind!='h' or not isinstance(h_transport_n,int) or h_transport_n<n or h_transport_n>64):raise ValueError('Unsupported H transport refinement')
    W=width;L=stub_ratio*W;mesh=Mesh(depth)
    if kind=='y':
        H=math.sqrt(3)/2*W;corners=np.array([[-W/2,-H/3],[W/2,-H/3],[0,2*H/3]])
        a,b,c=corners
        def p(i,j):return a+(b-a)*i/n+(c-a)*j/n
        for i in range(n):
            for j in range(n-i):
                mesh.polygon([p(i,j),p(i+1,j),p(i,j+1)],0)
                if i+j<=n-2:mesh.polygon([p(i+1,j),p(i+1,j+1),p(i,j+1)],0)
        for k,f in enumerate((1,.5,.75)):mesh.stub(corners[k],corners[(k+1)%3],L,n,k+1,f'y/port-{k}',f)
    elif kind in ['x','area']:
        corners=np.array([[-W/2,-W/2],[W/2,-W/2],[W/2,W/2],[-W/2,W/2]])
        mesh.rectangle(corners[0],[1,0],[0,1],W,W,n,n,0)
        for k in (range(4) if kind=='x' else [1,3]):
            f=.5 if kind=='area' and k==1 else 1
            mesh.stub(corners[k],corners[(k+1)%4],L,n,k+1,f'{kind}/port-{k}',f)
    elif kind=='straight':
        length=3*W
        mesh.rectangle([-length/2,-W/2],[1,0],[0,1],length,W,3*n,n,0)
        mesh.ports=[dict(id='straight/left',center=[-length/2,0],normal=[-1,0],width=W,area_m2=W*depth),dict(id='straight/right',center=[length/2,0],normal=[1,0],width=W,area_m2=W*depth)]
    else:
        for k,x in enumerate((-W,W)):
            corners=np.array([[x-W/2,-W/2],[x+W/2,-W/2],[x+W/2,W/2],[x-W/2,W/2]])
            mesh.rectangle(corners[0],[1,0],[0,1],W,W,n,n,k)
            for j in (0,2):mesh.stub(corners[j],corners[(j+1)%4],L,n,2+2*k+j//2,f'h/port-{k}-{j}',axial_n=h_transport_n if k==0 and j==2 else None)
        mesh.rectangle([-W/2,-W/2],[1,0],[0,1],W,W,h_transport_n if h_transport_n is not None else n,n,6)
    return mesh.prepared(f'{kind}-{n}'+(f'-transport-{h_transport_n}' if h_transport_n is not None else ''))

def native_input(mesh,mode='pulse',rate=2000000,temperature=300,source=0,amplitude=1,duration=.003,U=0):
    lines=[f'{len(mesh["cells"])} {len(mesh["faces"])} {len(mesh["ports"])} {mode} {rate:.17g} {temperature:.17g} {source} {amplitude:.17g} {duration:.17g} {U:.17g}']
    for c in mesh['cells']:lines.append(' '.join(map(str,[c['volume_m3'],*c['center_m'],c['region'],c['left_volume_fraction']])))
    for f in mesh['faces']:lines.append(' '.join(map(str,[f['a'],f['b'],f['port'],f['area_m2'],*f['normal'],*f['center_m']])))
    return '\n'.join(lines)+'\n'

def helmholtz(mesh,frequencies,sound_speed):
    """Independent P1 complex Helmholtz + local matched-impedance boundary.

    exp(i*w*t). Matrix assembly uses pressure basis functions, no candidate
    fluxes, time stepping, arrival adjustment or fitted attenuation.
    """
    from scipy.sparse import coo_matrix
    from scipy.sparse.linalg import splu
    xy=np.array(mesh['vertices_m']);N=len(xy);depth=mesh['depth_m'];ports=len(mesh['ports'])
    ii=[];jj=[];kk=[];mm=[]
    for ids in mesh['polygons']:
        triangles=[ids] if len(ids)==3 else [[ids[0],ids[1],ids[2]],[ids[0],ids[2],ids[3]]]
        for tri in triangles:
            p=xy[tri];matrix=np.column_stack([np.ones(3),p]);area=abs(np.linalg.det(matrix))/2
            basis=np.linalg.inv(matrix);grad=basis[1:,:]
            K=depth*area*grad.T@grad;M=depth*area/(12*sound_speed**2)*(np.ones((3,3))+np.eye(3))
            for a in range(3):
                for b in range(3):ii.append(tri[a]);jj.append(tri[b]);kk.append(K[a,b]);mm.append(M[a,b])
    K=coo_matrix((kk,(ii,jj)),shape=(N,N)).tocsc();M=coo_matrix((mm,(ii,jj)),shape=(N,N)).tocsc()
    bi=[];bj=[];bv=[];loads=np.zeros((N,ports))
    for f in mesh['faces']:
        if f['port']<0:continue
        ids=f['edge_vertices'];A=f['area_m2'];port=f['port']
        for a in range(2):
            loads[ids[a],port]+=A/2
            for b in range(2):bi.append(ids[a]);bj.append(ids[b]);bv.append(A*(2 if a==b else 1)/6)
    B=coo_matrix((bv,(bi,bj)),shape=(N,N)).tocsc();result=[]
    areas=np.array([p['area_m2'] for p in mesh['ports']])
    for f in frequencies:
        w=2*np.pi*f;solution=splu(K-w*w*M+1j*w/sound_speed*B).solve(2j*w/sound_speed*loads)
        pressure=(loads.T@solution)/areas[:,None]
        result.append(pressure-np.eye(ports))
    return np.array(result)

def compact_transfer(mesh,frequencies,sound_speed,with_volume=True):
    """Analytic compact plenums, retaining every physical stub and H bridge.

    Common uniform gas, exp(i*w*t), inviscid plane waves. This is a competing
    approximation, never the independent truth for a finite junction. Actual
    region volume/port area gives stub length; no fitted delay or gain.
    """
    kind=mesh['id'].split('-')[0]
    if kind not in ['y','x','h','area']:raise ValueError('Compact junction template required')
    volumes={}
    for cell in mesh['cells']:volumes[cell['region']]=volumes.get(cell['region'],0)+cell['volume_m3']
    ports=mesh['ports'];areas=np.array([p['area_m2'] for p in ports]);nodes=2 if kind=='h' else 1
    owners=np.array([i//2 if kind=='h' else 0 for i in range(len(ports))])
    stub_regions=[2+i if kind=='h' else int(p['id'].split('-')[-1])+1 for i,p in enumerate(ports)]
    lengths=np.array([volumes[r]/area for r,area in zip(stub_regions,areas)])
    cavity=np.array([volumes[i] if with_volume else 0 for i in range(nodes)])
    if kind=='h':bridge_area=areas[0];bridge_length=volumes[6]/bridge_area
    result=[]
    for frequency in frequencies:
        w=2*np.pi*frequency;k=w/sound_speed;delay=np.exp(-1j*k*lengths)
        matrix=np.diag(1j*w*cavity/sound_speed);rhs=np.zeros((nodes,len(ports)),complex)
        for p,owner in enumerate(owners):matrix[owner,owner]+=areas[p];rhs[owner,p]=2*areas[p]*delay[p]
        if kind=='h':
            phase=k*bridge_length;diag=-1j*bridge_area/np.tan(phase);cross=1j*bridge_area/np.sin(phase)
            matrix[0,0]+=diag;matrix[1,1]+=diag;matrix[0,1]+=cross;matrix[1,0]+=cross
        pressure=np.linalg.solve(matrix,rhs)
        result.append(delay[:,None]*(pressure[owners,:]-np.diag(delay)))
    return np.array(result)
