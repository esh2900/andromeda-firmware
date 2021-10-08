#ifndef MAGNETOMETRO_H
#define MAGNETOMETRO_H

#include "wmm.h"
#include "EGM9615.h"


double Declination(double lat, double _long,double alt, int day,int month, int year){
    MAGtype_MagneticModel * MagneticModels[1], *TimedMagneticModel;
    MAGtype_Ellipsoid Ellip;
    MAGtype_CoordSpherical CoordSpherical;
    MAGtype_CoordGeodetic CoordGeodetic;
    MAGtype_Date UserDate;
    MAGtype_GeoMagneticElements GeoMagneticElements;
    MAGtype_Geoid Geoid;
    char ans[20];
    char filename[] = "WMM.COF";
    char VersionDate[12];
    int NumTerms, nMax = 0;
    int epochs = 1;
    /* Memory allocation */

    strncpy(VersionDate, VERSIONDATE_LARGE + 39, 11);
    VersionDate[11] = '\0';
    if(!MAG_robustReadMagModels(filename, &MagneticModels, epochs)) {
        printf("\n WMM.COF not found.  Press enter to exit... \n ");
        fgets(ans, 20, stdin);
        return 1;
    }
    if(nMax < MagneticModels[0]->nMax) nMax = MagneticModels[0]->nMax;
    NumTerms = ((nMax + 1) * (nMax + 2) / 2);
    TimedMagneticModel = MAG_AllocateModelMemory(NumTerms); /* For storing the time modified WMM Model parameters */
    if(MagneticModels[0] == NULL || TimedMagneticModel == NULL)
    {
        MAG_Error(2);
    }
    MAG_SetDefaults(&Ellip, &Geoid); /* Set default values and constants */
    /* Check for Geographic Poles */



    /* Set EGM96 Geoid parameters */
    Geoid.GeoidHeightBuffer = GeoidHeightBuffer;
    Geoid.Geoid_Initialized = 1;
    /* Set EGM96 Geoid parameters END */
    MAG_GetUserInput(MagneticModels[0], &Geoid, &CoordGeodetic, &UserDate);
    MAG_GeodeticToSpherical(Ellip, CoordGeodetic, &CoordSpherical); /*Convert from geodetic to Spherical Equations: 17-18, WMM Technical report*/
    MAG_TimelyModifyMagneticModel(UserDate, MagneticModels[0], TimedMagneticModel); /* Time adjust the coefficients, Equation 19, WMM Technical report */
    MAG_Geomag(Ellip, CoordSpherical, CoordGeodetic, TimedMagneticModel, &GeoMagneticElements);

    double Decl=Get_declination(GeoMagneticElements);
    return Decl;
}

//Função que calcula o azimute do corpo com base na informação do magnetômetro
//recebe os parametros de posição e do acelerômetro
double azimut(double h_xmag,double h_ymag, double lat, double _long,double alt, int day,int month, int year){
    double azim=atan(h_ymag/h_xmag)+Declination(lat,_long,alt,day,month,year);
    return azim;
}

#endif