/* ============================================================
 * Author: F.J. Cruz <fj.cruz@supercable.es>
 * Date  : 2005-11-18
 * Copyright 2005-2006 by F.J. Cruz
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

// Littlecms library includes.

#include <config.h>
#include LCMS_HEADER

// QT includes

#include <qstring.h>
#include <qcstring.h>
#include <qfile.h>

// KDE includes

#include <kdebug.h>

// Local includes

#include "icctransform.h"

namespace Digikam
{

class IccTransformPriv
{
public:

    IccTransformPriv()
    {
        has_profile      = false;
        do_proof_profile = false;
    }

    bool       do_proof_profile;
    bool       has_profile; 
    
    QString    input_profile;
    QString    output_profile;
    QString    proof_profile;
    
    QByteArray embedded_profile;
};

IccTransform::IccTransform()
{
    d = new IccTransformPriv;
}

IccTransform::~IccTransform()
{
    delete d;
}

void IccTransform::getTransformType(bool do_proof_profile)
{
    if (do_proof_profile)
    {
        d->do_proof_profile = true;
    }
    else
    {
        d->do_proof_profile = false;
    }
}

void IccTransform::getEmbeddedProfile(DImg image)
{
    if (!image.getICCProfil().isNull())
    {
        d->embedded_profile = image.getICCProfil();
        d->has_profile = true;
        kdDebug() << "Has profile" << endl;
    }
    else
    {
        return;
    }
}

void IccTransform::setProfiles(QString input_profile, QString output_profile)
{
    d->input_profile  = input_profile;
    d->output_profile = output_profile;
}

void IccTransform::setProfiles(QString input_profile, QString output_profile, 
                               QString proof_profile)
{
    d->input_profile  = input_profile;
    d->output_profile = output_profile;
    d->proof_profile  = proof_profile;
}

void IccTransform::setProfiles(QString output_profile)
{
    d->output_profile = output_profile;
}

QString IccTransform::getEmbeddedProfileDescriptor()
{
kdDebug() << "First open embedded profile" << endl;
    cmsHPROFILE tmpProfile = cmsOpenProfileFromMem(d->embedded_profile.data(), (DWORD)d->embedded_profile.size());
    QString embeddedProfileDescriptor =QString(cmsTakeProductDesc(tmpProfile));
    cmsCloseProfile(tmpProfile);
    return embeddedProfileDescriptor;
}

void IccTransform::apply(DImg& image)
{
    cmsHPROFILE   inprofile=0, outprofile=0, proofprofile=0;
    cmsHTRANSFORM transform;

    if (d->has_profile)
    {
        kdDebug() << "Second open embedded profile" << endl;
        inprofile = cmsOpenProfileFromMem(d->embedded_profile.data(),
                                          (DWORD)d->embedded_profile.size());
//         embeddedProfileDescriptor = QString(cmsTakeProductDesc(inprofile));
        kdDebug() << "Embedded profile name: " << cmsTakeProductDesc(inprofile) << endl;
    }
    else
    {
        inprofile = cmsOpenProfileFromFile(QFile::encodeName( d->input_profile ), "r");
    }

    outprofile = cmsOpenProfileFromFile(QFile::encodeName( d->output_profile ), "r");

    if (!d->do_proof_profile)
    {
        if (image.sixteenBit())
        {
            if (image.hasAlpha())
            {
                transform = cmsCreateTransform( inprofile,
                                                TYPE_BGRA_16,
                                                outprofile,
                                                TYPE_BGRA_16,
                                                INTENT_PERCEPTUAL,
                                                cmsFLAGS_WHITEBLACKCOMPENSATION);
            }
            else
            {
                transform = cmsCreateTransform( inprofile,
                                                TYPE_BGR_16,
                                                outprofile,
                                                TYPE_BGR_16,
                                                INTENT_PERCEPTUAL,
                                                cmsFLAGS_WHITEBLACKCOMPENSATION);
            }

        }
        else
        {
            if (image.hasAlpha())
            {
                transform = cmsCreateTransform( inprofile,
                                                TYPE_BGRA_8,
                                                outprofile,
                                                TYPE_BGRA_8,
                                                INTENT_PERCEPTUAL,
                                                cmsFLAGS_WHITEBLACKCOMPENSATION);
            }
            else
            {
                transform = cmsCreateTransform( inprofile,
                                                TYPE_BGR_8,
                                                outprofile,
                                                TYPE_BGR_8,
                                                INTENT_PERCEPTUAL,
                                                cmsFLAGS_WHITEBLACKCOMPENSATION);
            }

        }
    }
    else
    {
        proofprofile = cmsOpenProfileFromFile(QFile::encodeName( d->proof_profile ), "r");

        if (image.sixteenBit())
        {
            if (image.hasAlpha())
            {
                transform = cmsCreateProofingTransform( inprofile,
                                                        TYPE_BGRA_16,
                                                        outprofile,
                                                        TYPE_BGRA_16,
                                                        proofprofile,
                                                        INTENT_ABSOLUTE_COLORIMETRIC,
                                                        INTENT_ABSOLUTE_COLORIMETRIC,
                                                        cmsFLAGS_WHITEBLACKCOMPENSATION);
            }
            else
            {
                transform = cmsCreateProofingTransform( inprofile,
                                                        TYPE_BGR_16,
                                                        outprofile,
                                                        TYPE_BGR_16,
                                                        proofprofile,
                                                        INTENT_ABSOLUTE_COLORIMETRIC,
                                                        INTENT_ABSOLUTE_COLORIMETRIC,
                                                        cmsFLAGS_WHITEBLACKCOMPENSATION);
            }
        }
        else
        {
            if (image.hasAlpha())
            {
                transform = cmsCreateProofingTransform( inprofile,
                                                        TYPE_BGR_8,
                                                        outprofile,
                                                        TYPE_BGR_8,
                                                        proofprofile,
                                                        INTENT_ABSOLUTE_COLORIMETRIC,
                                                        INTENT_ABSOLUTE_COLORIMETRIC,
                                                        cmsFLAGS_WHITEBLACKCOMPENSATION);
            }
            else
            {
                transform = cmsCreateProofingTransform( inprofile,
                                                        TYPE_BGR_8,
                                                        outprofile,
                                                        TYPE_BGR_8,
                                                        proofprofile,
                                                        INTENT_ABSOLUTE_COLORIMETRIC,
                                                        INTENT_ABSOLUTE_COLORIMETRIC,
                                                        cmsFLAGS_WHITEBLACKCOMPENSATION);
            }
        }
    }

     // We need to work using temp pixel buffer to apply ICC transformations.
    uchar  transdata[image.bytesDepth()];
    
    // Always working with uchar* prevent endianess problem.
    uchar *data = image.bits();

    // We scan all image pixels one by one.
    for (uint i=0; i < image.width()*image.height()*image.bytesDepth(); i+=image.bytesDepth())
    {
        // Apply ICC transformations.
        cmsDoTransform( transform, &data[i], &transdata[0], 1);
        
        // Copy buufer to source to update original image with ICC corrections.
        // Alpha channel is restored in all cases.
        memcpy (&data[i], &transdata[0], (image.bytesDepth() == 8) ? 6 : 3);        
    }
    
    cmsDeleteTransform(transform);
    cmsCloseProfile(inprofile);
    cmsCloseProfile(outprofile);
    
    if (d->do_proof_profile)
       cmsCloseProfile(proofprofile);
}

QString IccTransform::getProfileDescription(QString profile)
{

    cmsHPROFILE _profile = cmsOpenProfileFromFile(QFile::encodeName(profile), "r");
    QString _description = cmsTakeProductDesc(_profile);
    cmsCloseProfile(_profile);
    return _description;

}

}  // NameSpace Digikam
