FMUS_DIR=`pwd`

#undisturbed output
mpiexec -np 5 fmigo-mpi -t 60 -d 0.01 \
        -p i,0,4,1 \
        -p i,0,5,10000 \
        -p r,0,7,1 \
        -p r,0,8,1 \
        -C shaft,0,1,0,1,2,3,0,1,2,3 \
        -C shaft,1,2,5,6,7,8,0,1,2,3 \
        -C shaft,2,3,6,7,8,9,0,1,2,3 \
        ${FMUS_DIR}/impulse/impulse.fmu \
        ${FMUS_DIR}/kinematictruck/kinclutch/kinclutch.fmu \
        ${FMUS_DIR}/kinematictruck/gearbox2/gearbox2.fmu \
        ${FMUS_DIR}/kinematictruck/body/body.fmu  > out1.csv

#impulse applied at t=30 (-p i,0,5,3000 option)
mpiexec -np 5 fmigo-mpi -t 60 -d 0.01 \
        -p i,0,4,1 \
        -p i,0,5,3000 \
        -p r,0,7,1 \
        -p r,0,8,1 \
        -C shaft,0,1,0,1,2,3,0,1,2,3 \
        -C shaft,1,2,5,6,7,8,0,1,2,3 \
        -C shaft,2,3,6,7,8,9,0,1,2,3 \
        ${FMUS_DIR}/impulse/impulse.fmu \
        ${FMUS_DIR}/kinematictruck/kinclutch/kinclutch.fmu \
        ${FMUS_DIR}/kinematictruck/gearbox2/gearbox2.fmu \
        ${FMUS_DIR}/kinematictruck/body/body.fmu  > out2.csv

octave --no-gui --persist --eval "d1=load('out1.csv' ); d2=load('out2.csv' ); dd=d2-d1; dd(:,1)=d1(:,1);  plot(dd(3000:end,1)-30,dd(3000:end,2:end))"
