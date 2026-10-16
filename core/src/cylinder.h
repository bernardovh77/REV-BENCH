/*
 * cylinder.h - one cylinder: the slider-crank mechanism and the gas above
 * the piston.
 *
 *       ____________
 *      |   gas      |   V = clearance volume + piston area * x
 *      |____________|
 *      |  piston    |   x = distance of the piston below TDC
 *      |____ o _____|   wrist pin
 *            \
 *             \         connecting rod, length l
 *              o        crank pin, at crank radius a = stroke / 2
 *             /  theta  crank angle, measured from TDC
 *            O          crankshaft axis
 *
 * Crank angle convention, used everywhere (including the 3D view later):
 * theta is measured from top dead centre (TDC). Over the 720-degree
 * four-stroke cycle, 0 = TDC at the start of the power stroke ("firing TDC"),
 * 180 = BDC, 360 = TDC between exhaust and intake ("overlap TDC"), 540 = BDC.
 * The mechanism itself repeats every 360 degrees.
 */
#ifndef RB_CYLINDER_H
#define RB_CYLINDER_H

#include "chamber.h"

typedef struct rb_cyl_geom {
    /* inputs */
    double bore;      /* m, cylinder diameter                         */
    double stroke;    /* m, piston travel from TDC to BDC             */
    double rod;       /* m, connecting rod length, centre to centre   */
    double cr;        /* compression ratio: largest / smallest volume */
    /* derived by rb_cyl_geom_init() */
    double area;      /* m^2, piston area pi B^2 / 4                  */
    double crank_r;   /* m, crank radius a = stroke / 2               */
    double v_disp;    /* m^3, swept (displaced) volume area * stroke  */
    double v_clear;   /* m^3, clearance volume left at TDC            */
} rb_cyl_geom;

/* Fills g from the four inputs. Returns 0, or -1 for an impossible engine:
 * sizes that are not positive (or absurd: bore or stroke of 10 m or more), a
 * rod no longer than the stroke, or a compression ratio outside (1, 100). */
int    rb_cyl_geom_init(rb_cyl_geom *g, double bore, double stroke, double rod, double cr);

double rb_cyl_piston_x(const rb_cyl_geom *g, double theta);    /* m below TDC           */
double rb_cyl_piston_dx(const rb_cyl_geom *g, double theta);   /* dx/dtheta, m/rad      */
double rb_cyl_volume(const rb_cyl_geom *g, double theta);      /* gas volume, m^3       */
double rb_cyl_dvolume(const rb_cyl_geom *g, double theta);     /* dV/dtheta, m^3/rad    */

typedef struct rb_cylinder {
    rb_cyl_geom geom;
    rb_chamber  gas;     /* the gas above the piston                           */
    double      theta;   /* crank angle, rad                                    */
    double      work;    /* J done on the gas by the piston in the last move:
                          * positive compressing, negative when the gas pushes */
} rb_cylinder;

/* A cylinder at crank angle theta, filled with gas at (p, T) of composition
 * y. Returns 0, or -1 for an impossible geometry. */
int    rb_cylinder_init(rb_cylinder *c, const rb_cyl_geom *geom, double theta,
                        double p, double T, const double y[RB_NSPECIES]);

/* Turns the crank to theta (rad, absolute) and changes the gas volume to
 * match (see rb_chamber_set_volume). Returns the work done on the gas (J). */
double rb_cylinder_move(rb_cylinder *c, double theta);

#endif /* RB_CYLINDER_H */
