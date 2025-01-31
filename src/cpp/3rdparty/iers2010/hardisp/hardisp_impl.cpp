// #include "hardisp.hpp"
#include "3rdparty/iers2010/iers2010.hpp"
#include <cstring>
#include <fstream>
//#include <iostream>
#include <stdexcept>

using iers2010::hisp::nl;
using iers2010::hisp::nt;
using iers2010::hisp::ntin;

/// @brief Read in amplitudes and phases, in standard "Scherneck" form
///
/// @param[in] tamp Matrix of size 3xNTIN; the first three lines of the
///                coefficients will be read in here (aka the amplitudes)
/// @param[in] tph  Matrix of size 3xNTIN; lines 4 to 6 will be read in here
///                (aka the phase coefficients)
/// @filename       The filename of the BLQ file to be read in; if none is
/// given,
///                then the function expects to read in from STDIN
/// @return         Anything other than 0 denotes an error
///
/// @warning       Note that the function will change sign for phase, to be
///                negative for lags
int iers2010::hisp::read_hardisp_args(double tamp[3][ntin], double tph[3][ntin],
                                      const char *filename) {
  std::istream *in;
  std::ifstream ifn;
  int sta2read = 1;

  if (!filename) {
    in = &std::cin;
  } else {
    ifn.open(filename);
    in = &ifn;
    if (!ifn.is_open()) {
      fprintf(stderr, "[ERROR] Failed to open file \"%s\" (traceback: %s)\n",
              filename, __func__);
      return 1;
    }
  }

  //  Read in amplitudes and phases, in standard "Scherneck" form, from
  //  standard input
  while (sta2read > 0 && in->good()) {
    for (int i = 0; i < 3; i++) {
      for (int kk = 0; kk < ntin; kk++) {
        *in >> tamp[i][kk];
      }
    }

    for (int i = 0; i < 3; i++) {
      for (int kk = 0; kk < ntin; kk++) {
        *in >> tph[i][kk];
      }
      // Change sign for phase, to be negative for lags
      for (int kk = 0; kk < ntin; kk++) {
        tph[i][kk] = -tph[i][kk];
      }
    }

    --sta2read;
  }

  return !(in->good());
}

/// @details This program reads in a file of station displacements in the BLQ
///          format used by Scherneck and Bos for ocean loading, and outputs a
///          time series of computed tidal displacements, using an expanded set
///          of tidal constituents, whose amplitudes and phases are found by
///          spline interpolation of the tidal admittance.  A total of 342
///          constituent tides are included, which gives a precision of about
///          0.1%.
///          This function is a translation/wrapper for the fortran HARDISP
///          subroutine, found here :
///          http://maia.usno.navy.mil/conv2010/software.html
/// @param[in] irnt Number of output samples (of samp seconds)
/// @param[in] samp Sample time interval in seconds
/// @param[out] du Array of size irnt; at output holds the values of radial
///                tidal ocean loading displacement
/// @param[out] dw Array of size irnt; at output holds the values of west
///                tidal ocean loading displacement
/// @param[out] ds Array of size irnt; at output holds the values of south
///                tidal ocean loading displacement
/// @return        An integer denoting the exit status:
///                Returned Value  | Function Status
///                ----------------|-------------------------------------
///                              0 | Sucess
///
/// @note
///    -# The input ocean loading coefficients were generated by the ocean
///       loading service on 25 June 2009 using
///       http://www.oso.chalmers.se/~loading/ for IGS stations Onsala and
///       Reykjavik using the CSR4.0 model and "NO" geocenter correction.
///    -# The site displacement output is written to standard output with the
///       format 3F14.6. All units are expressed in meters. The numbers written
///       correspong to: <br>
///       du : Radial tidal ocean loading displacement,<br>
///       dw : West tidal ocean loading displacement,<br>
///       ds : South tidal ocean loading displacement
///    -# \c argv and \c argc are read from stdin (like in every 'main'). Date
///      provided, must be in UTC scale.
///    -# irnt and samp define the computation of tidal displacements; actually,
///       we are computing irnt values for every samp seconds begining at the
///       epoch provided. E.g. given: datetime_start, irnt=24, samp=3600 means
///       that we are computing displacements from datetime_start and for every
///       hour, for a total of 24 hours (aka up untill the next day).
///    -# Status:  Class 1 model
///
/// @warning \b IMPORTANT <br>
///    A new version of this routine must be
///    produced whenever a new leap second is
///    announced.  There are three items to
///    change on each such occasion:<br>
///    1) Update the nstep variable<br>
///    2) Update the arrays st and si<br>
///    3) Change date of latest leap second<br>
///    <b>Latest leap second:  2016 December 31</b>
///
/// @version 19.12.2016
///
/// @cite iers2010
int iers2010::hisp::hardisp_impl(int irnt, double samp, double tamp[3][ntin],
                                 double tph[3][ntin],
                                //  dso::datetime<dso::seconds> epoch, double *odu,
                                 GTime epoch, double *odu,
                                 double *ods, double *odw) {
  constexpr double dr = 0.01745329252e0;
  int irli = 1;

#ifdef USE_EXTERNAL_CONSTS
  constexpr double PI(iers2010::DPI);
#else
  constexpr double PI(3.1415926535897932384626433e0);
#endif

  /*+---------------------------------------------------------------------
   *  Find amplitudes and phases for all constituents, for each of the
   *  three displacements. Note that the same frequencies are returned
   *  each time.
   *
   *  BLQ format order is vertical, horizontal EW, horizontal NS
   *----------------------------------------------------------------------*/
  double az[nt], pz[nt], f[nt], aw[nt], pw[nt], as[nt], ps[nt];
  int ntout;
  double amp[ntin], phase[ntin];

  for (int i = 0; i < ntin; i++) {
    amp[i] = tamp[0][i];
    phase[i] = tph[0][i];
  }
  iers2010::hisp::admint(amp, phase, epoch, az, f, pz, ntin, ntout);

  for (int i = 0; i < ntin; i++) {
    amp[i] = tamp[1][i];
    phase[i] = tph[1][i];
  }
  iers2010::hisp::admint(amp, phase, epoch, aw, f, pw, ntin, ntout);

  for (int i = 0; i < ntin; i++) {
    amp[i] = tamp[2][i];
    phase[i] = tph[2][i];
  }
  iers2010::hisp::admint(amp, phase, epoch, as, f, ps, ntin, ntout);

  // set up for recursion, by normalizing frequencies, and converting
  // phases to radians
  double wf[nt];
  for (int i = 0; i < ntout; i++) {
    pz[i] = dr * pz[i];
    ps[i] = dr * ps[i];
    pw[i] = dr * pw[i];
    f[i] = samp * PI * f[i] / 43200e0;
    wf[i] = f[i];
  }

  /*+---------------------------------------------------------------------
   *
   *  Loop over times, nl output points at a time. At the start of each
   *  such block, convert from amp and phase to sin and cos (hc array) at
   *  the start of the block. The computation of values within each
   *  block is done recursively, since the times are equi-spaced.
   *
   *  The loop will perform more than one iterations if irnt > nl(=600)
   *
   *----------------------------------------------------------------------*/
  int iteration = 0;
  while (true) {
    int irhi(std::min(irli + nl - 1, irnt));
    int np(irhi - irli + 1);

    // Set up harmonic coefficients, compute tide, and write out
    double hcz[2 * nt + 1], hcs[2 * nt + 1], hcw[2 * nt + 1];
    for (int i = 0; i < nt; i++) {
      hcz[2 * i] = az[i] * cos(pz[i]);
      hcz[2 * i + 1] = -az[i] * sin(pz[i]);
      hcs[2 * i] = as[i] * cos(ps[i]);
      hcs[2 * i + 1] = -as[i] * sin(ps[i]);
      hcw[2 * i] = aw[i] * cos(pw[i]);
      hcw[2 * i + 1] = -aw[i] * sin(pw[i]);
    }

    double dz[nl], ds[nl], dw[nl];
    double scr[3 * nt];
    recurs(dz, np, hcz, ntout, wf, scr);
    recurs(ds, np, hcs, ntout, wf, scr);
    recurs(dw, np, hcw, ntout, wf, scr);

    std::memcpy(odu + iteration * nl, dz, sizeof(double) * np);
    std::memcpy(ods + iteration * nl, ds, sizeof(double) * np);
    std::memcpy(odw + iteration * nl, dw, sizeof(double) * np);

    if (irhi == irnt)
      break;

    irli = irhi + 1;

    // Reset phases to the start of the new section
    for (int i = 0; i < nt; i++) {
      pz[i] = fmod(pz[i] + np * f[i], 2e0 * PI);
      ps[i] = fmod(ps[i] + np * f[i], 2e0 * PI);
      pw[i] = fmod(pw[i] + np * f[i], 2e0 * PI);
    }
    ++iteration;
  }

  return 0;
}
