/* ============================================================
 * @file
 *
 * This file is a part of digiKam
 * <a href="http://www.digikam.org">http://www.digikam.org</a>
 *
 * Date        : 2010-06-16
 * Description : A convenience class for a standalone face detector
 *
 * Copyright (C)      2010 by Marcel Wiesweg <marcel dot wiesweg at gmx dot de>
 * Copyright (C)      2010 by Aditya Bhatt <adityabhatt1991 at gmail dot com>
 * Copyright (C) 2010-2017 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

// OpenCV includes need to show up before Qt includes
#include "opencvfisherfacerecognizer.h"
#include "opencveigenfacerecognizer.h"
#include "opencvlbphfacerecognizer.h"
#include "opencvdnnfacerecognizer.h"
#include "funnelreal.h"

// Qt includes

#include <QMutex>
#include <QMutexLocker>
#include <QUuid>
#include <QDir>
#include <QStandardPaths>

// Local includes

#include "coredbaccess.h"
#include "dbengineparameters.h"
#include "facedbaccess.h"
#include "facedboperationgroup.h"
#include "dataproviders.h"
#include "recognitiondatabase.h"
#include "algs.h"
#include "facedb.h"
#include "digikam_debug.h"

namespace Digikam
{

/** Simple QImage training data container used by RecognitionDatabase::train(Identity, QImage, QString)
 */
class SimpleTrainingDataProvider : public TrainingDataProvider
{
public:

    SimpleTrainingDataProvider(const Identity& identity, const QList<QImage>& newImages)
        : identity(identity),
          toTrain(newImages)
    {
    }

    ImageListProvider* newImages(const Identity& id)
    {
        if (identity == id)
        {
            toTrain.reset();
            return &toTrain;
        }

        return &empty;
    }

    ImageListProvider* images(const Identity&)
    {
        return &empty;
    }

public:

    Identity               identity;
    QListImageListProvider toTrain;
    QListImageListProvider empty;
};

// -----------------------------------------------------------------------------------------------

class RecognitionDatabase::Private
{
public:

    //for recognize algorithm option
    enum RecognizeAlgorithm
    {
        LBP,
        EigenFace,
        FisherFace,
        DNN
    };

    bool                 dbAvailable;
    QMutex               mutex;
    QVariantMap          parameters;
    QHash<int, Identity> identityCache;
    RecognizeAlgorithm   recognizeAlgorithm;

public:

    Private();
    ~Private();

public:

    template <class T>
    T* getObjectOrCreate(T* &ptr) const
    {
        if (!ptr)
        {
            qCDebug(DIGIKAM_FACESENGINE_LOG) << "create recognizer";
            ptr = new T();
        }

        return ptr;
    }

public:

    // Change these three lines to change CurrentRecognizer
    typedef OpenCVLBPHFaceRecognizer CurrentRecognizer;

    CurrentRecognizer* recognizer()             { return getObjectOrCreate(opencvlbph); }
    CurrentRecognizer* recognizerConst()  const { return opencvlbph;                    }

public:

    OpenCVLBPHFaceRecognizer* lbph()                { return getObjectOrCreate(opencvlbph);  }
    OpenCVLBPHFaceRecognizer* lbphConst() const     { return opencvlbph;                     }

    OpenCVEIGENFaceRecognizer* eigen()              { return getObjectOrCreate(opencveigen); }
    OpenCVEIGENFaceRecognizer* eigenConst() const   { return opencveigen;                    }

    OpenCVFISHERFaceRecognizer* fisher()            { return getObjectOrCreate(opencvfisher); }
    OpenCVFISHERFaceRecognizer* fisherConst() const { return opencvfisher;                    }

    OpenCVDNNFaceRecognizer* dnn()                  { return getObjectOrCreate(opencvdnn); }
    OpenCVDNNFaceRecognizer* dnnConst() const       { return opencvdnn;                    }

public:

    typedef FunnelReal CurrentAligner;

    CurrentAligner*    aligner();
    CurrentAligner*    alignerConst()     const { return funnel;                        }

public:

    void applyParameters();

    void train(OpenCVLBPHFaceRecognizer* const r, const QList<Identity>& identitiesToBeTrained,
               TrainingDataProvider* const data, const QString& trainingContext);
    void train(OpenCVEIGENFaceRecognizer* const r, const QList<Identity>& identitiesToBeTrained,
               TrainingDataProvider* const data, const QString& trainingContext);
    void clear(OpenCVLBPHFaceRecognizer* const, const QList<int>& idsToClear, const QString& trainingContext);
    void clear(OpenCVEIGENFaceRecognizer* const, const QList<int>& idsToClear, const QString& trainingContext);
    void clear(OpenCVFISHERFaceRecognizer* const, const QList<int>& idsToClear, const QString& trainingContext);
    void clear(OpenCVDNNFaceRecognizer* const, const QList<int>& idsToClear, const QString& trainingContext);

    cv::Mat preprocessingChain(const QImage& image);
    cv::Mat preprocessingChainRGB(const QImage& image);

public:

    bool identityContains(const Identity& identity, const QString& attribute, const QString& value) const;
    Identity findByAttribute(const QString& attribute, const QString& value) const;
    Identity findByAttributes(const QString& attribute, const QMap<QString, QString>& valueMap) const;

private:

    OpenCVFISHERFaceRecognizer* opencvfisher;
    OpenCVEIGENFaceRecognizer*  opencveigen;
    OpenCVLBPHFaceRecognizer*   opencvlbph;
    OpenCVDNNFaceRecognizer*    opencvdnn;

    FunnelReal*                 funnel;
};

// ----------------------------------------------------------------------------------------------

RecognitionDatabase::Private::Private()
    : mutex(QMutex::Recursive),
      opencvfisher(0),
      opencveigen(0),
      opencvlbph(0),
      opencvdnn(0),
      funnel(0)
{
    DbEngineParameters params = CoreDbAccess::parameters().faceParameters();
    params.setFaceDatabasePath(CoreDbAccess::parameters().faceParameters().getFaceDatabaseNameOrDir());
    FaceDbAccess::setParameters(params);
    dbAvailable               = FaceDbAccess::checkReadyForUse(0);
    recognizeAlgorithm        = RecognizeAlgorithm::LBP;

    if (dbAvailable)
    {
        qCDebug(DIGIKAM_FACESENGINE_LOG) << "Face database ready for use";

        foreach (const Identity& identity, FaceDbAccess().db()->identities())
        {
            identityCache[identity.id()] = identity;
        }
    }
    else
    {
        qCDebug(DIGIKAM_FACESENGINE_LOG) << "Failed to initialize face database";
    }
}

RecognitionDatabase::Private::~Private()
{
    delete opencvfisher;
    delete opencveigen;
    delete opencvlbph;
    delete opencvdnn;
    delete funnel;
}

RecognitionDatabase::Private::CurrentAligner* RecognitionDatabase::Private::aligner()
{
    if (!funnel)
    {
        funnel = new FunnelReal;
    }

    return funnel;
}

// NOTE: other RecognitionDatabase::Private methods are to be found below, in the relevant context of the main class

// -------------------------------------------------------------------------------------------------

RecognitionDatabase::RecognitionDatabase()
    : d(new Private)
{
}

RecognitionDatabase::~RecognitionDatabase()
{
    delete d;
}

QList<Identity> RecognitionDatabase::allIdentities() const
{
    if (!d || !d->dbAvailable)
        return QList<Identity>();

    QMutexLocker lock(&d->mutex);

    return (d->identityCache.values());
}

Identity RecognitionDatabase::identity(int id) const
{
    if (!d || !d->dbAvailable)
    {
        return Identity();
    }

    QMutexLocker lock(&d->mutex);

    return (d->identityCache.value(id));
}

// Takes care that there may be multiple values of attribute in identity's attributes
bool RecognitionDatabase::Private::identityContains(const Identity& identity, const QString& attribute, const QString& value) const
{
    const QMap<QString, QString> map          = identity.attributesMap();
    QMap<QString, QString>::const_iterator it = map.constFind(attribute);

    for (; it != map.constEnd() && it.key() == attribute; ++it)
    {
        if (it.value() == value)
        {
            return true;
        }
    }

    return false;
}

Identity RecognitionDatabase::Private::findByAttribute(const QString& attribute, const QString& value) const
{
    foreach (const Identity& identity, identityCache)
    {
        if (identityContains(identity, attribute, value))
        {
            return identity;
        }
    }

    return Identity();
}

// NOTE: Takes care that there may be multiple values of attribute in valueMap
Identity RecognitionDatabase::Private::findByAttributes(const QString& attribute, const QMap<QString, QString>& valueMap) const
{
    QMap<QString, QString>::const_iterator it = valueMap.find(attribute);

    for (; it != valueMap.end() && it.key() == attribute; ++it)
    {
        foreach (const Identity& identity, identityCache)
        {
            if (identityContains(identity, attribute, it.value()))
            {
                return identity;
            }
        }
    }

    return Identity();
}

Identity RecognitionDatabase::findIdentity(const QString& attribute, const QString& value) const
{
    if (!d || !d->dbAvailable || attribute.isEmpty())
    {
        return Identity();
    }

    QMutexLocker lock(&d->mutex);

    return (d->findByAttribute(attribute, value));
}

Identity RecognitionDatabase::findIdentity(const QMap<QString, QString>& attributes) const
{
    if (!d || !d->dbAvailable || attributes.isEmpty())
    {
        return Identity();
    }

    QMutexLocker lock(&d->mutex);

    Identity match;

    // First and foremost, UUID
    QString uuid = attributes.value(QString::fromLatin1("uuid"));
    match        = d->findByAttribute(QString::fromLatin1("uuid"), uuid);

    if (!match.isNull())
    {
        return match;
    }

    // A negative UUID match, with a given UUID, precludes any further search
    if (!uuid.isNull())
    {
        return Identity();
    }

    // full name
    match = d->findByAttributes(QString::fromLatin1("fullName"), attributes);

    if (!match.isNull())
    {
        return match;
    }

    // name
    match = d->findByAttributes(QString::fromLatin1("name"), attributes);

    if (!match.isNull())
    {
        return match;
    }

    QMap<QString, QString>::const_iterator it;

    for (it = attributes.begin(); it != attributes.end(); ++it)
    {
        if (it.key() == QString::fromLatin1("uuid")     ||
            it.key() == QString::fromLatin1("fullName") ||
            it.key() == QString::fromLatin1("name"))
        {
            continue;
        }

        match = d->findByAttribute(it.key(), it.value());

        if (!match.isNull())
        {
            return match;
        }
    }

    return Identity();
}

Identity RecognitionDatabase::addIdentity(const QMap<QString, QString>& attributes)
{
    if (!d || !d->dbAvailable)
    {
        return Identity();
    }

    QMutexLocker lock(&d->mutex);

    if (attributes.contains(QString::fromLatin1("uuid")))
    {
        Identity matchByUuid = findIdentity(QString::fromLatin1("uuid"), attributes.value(QString::fromLatin1("uuid")));

        if (!matchByUuid.isNull())
        {
            // This situation is not well defined.

            qCDebug(DIGIKAM_FACESENGINE_LOG) << "Called addIdentity with a given UUID, and there is such a UUID already in the database."
                                             << "The existing identity is returned without adjusting properties!";

            return matchByUuid;
        }
    }

    Identity identity;
    {
        FaceDbOperationGroup group;
        int id = FaceDbAccess().db()->addIdentity();
        identity.setId(id);
        identity.setAttributesMap(attributes);
        identity.setAttribute(QString::fromLatin1("uuid"), QUuid::createUuid().toString());
        FaceDbAccess().db()->updateIdentity(identity);
    }

    d->identityCache[identity.id()] = identity;

    return identity;
}

void RecognitionDatabase::addIdentityAttributes(int id, const QMap<QString, QString>& attributes)
{
    if (!d || !d->dbAvailable)
    {
        return;
    }

    QMutexLocker lock(&d->mutex);

    QHash<int, Identity>::iterator it = d->identityCache.find(id);

    if (it != d->identityCache.end())
    {
        QMap<QString, QString> map = it->attributesMap();
        map.unite(attributes);
        it->setAttributesMap(map);
        FaceDbAccess().db()->updateIdentity(*it);
    }
}

void RecognitionDatabase::addIdentityAttribute(int id, const QString& attribute, const QString& value)
{
    if (!d || !d->dbAvailable)
    {
        return;
    }

    QMutexLocker lock(&d->mutex);
    QHash<int, Identity>::iterator it = d->identityCache.find(id);

    if (it != d->identityCache.end())
    {
        QMap<QString, QString> map = it->attributesMap();
        map.insertMulti(attribute, value);
        it->setAttributesMap(map);
        FaceDbAccess().db()->updateIdentity(*it);
    }
}

void RecognitionDatabase::setIdentityAttributes(int id, const QMap<QString, QString>& attributes)
{
    if (!d || !d->dbAvailable)
    {
            return;
    }

    QMutexLocker lock(&d->mutex);
    QHash<int, Identity>::iterator it = d->identityCache.find(id);

    if (it != d->identityCache.end())
    {
        it->setAttributesMap(attributes);
        FaceDbAccess().db()->updateIdentity(*it);
    }
}

QString RecognitionDatabase::backendIdentifier() const
{
    if (d->recognizeAlgorithm == Private::RecognizeAlgorithm::LBP)
    {
        return QString::fromLatin1("opencvlbph");
    }
    else if (d->recognizeAlgorithm == Private::RecognizeAlgorithm::EigenFace)
    {
        return QString::fromLatin1("eigenfaces");
    }
    else if (d->recognizeAlgorithm == Private::RecognizeAlgorithm::FisherFace)
    {
        return QString::fromLatin1("fisherfaces");
    }

    // d->recognizeAlgorithm == Private::RecognizeAlgorithm::DNN
    return QString::fromLatin1("dnn");
}

void RecognitionDatabase::Private::applyParameters()
{
    if (recognizerConst())
    {
        for (QVariantMap::const_iterator it = parameters.constBegin(); it != parameters.constEnd(); ++it)
        {
            if (it.key() == QString::fromLatin1("threshold") || it.key() == QString::fromLatin1("accuracy"))
            {
                recognizer()->setThreshold(it.value().toFloat());
            }
        }
    }
}

void RecognitionDatabase::setParameter(const QString& parameter, const QVariant& value)
{
    if (!d || !d->dbAvailable)
    {
        return;
    }

    QMutexLocker lock(&d->mutex);

    d->parameters.insert(parameter, value);
    d->applyParameters();
}

void RecognitionDatabase::setParameters(const QVariantMap& parameters)
{
    if (!d || !d->dbAvailable)
    {
        return;
    }

    QMutexLocker lock(&d->mutex);

    for (QVariantMap::const_iterator it = parameters.begin(); it != parameters.end(); ++it)
    {
        d->parameters.insert(it.key(), it.value());
    }

    d->applyParameters();
}

QVariantMap RecognitionDatabase::parameters() const
{
    if (!d || !d->dbAvailable)
    {
        return QVariantMap();
    }

    QMutexLocker lock(&d->mutex);

    return d->parameters;
}

int RecognitionDatabase::recommendedImageSize(const QSize& availableSize) const
{
    // hardcoded for now, change when we know better.
    Q_UNUSED(availableSize)

    return 256;
}

Identity RecognitionDatabase::recognizeFace(const QImage& image)
{
    QList<Identity> result = recognizeFaces(QList<QImage>() << image);

    if (result.isEmpty())
        return Identity();

    return result.first();
}

QList<Identity> RecognitionDatabase::recognizeFaces(const QList<QImage>& images)
{
    QListImageListProvider provider(images);

    return recognizeFaces(&provider);
}

cv::Mat RecognitionDatabase::Private::preprocessingChain(const QImage& image)
{
    try
    {
        //cv::Mat cvImage = recognizer()->prepareForRecognition(image);
        cv::Mat cvImage;
        cvImage = lbph()->prepareForRecognition(image);
        /*
        if(recognizeAlgorithm==RecognizeAlgorithm::LBP)
        {
            cvImage = lbph()->prepareForRecognition(image);
        }
        else if(recognizeAlgorithm==RecognizeAlgorithm::EigenFace)
        {
            cvImage = eigen()->prepareForRecognition(image);
        }
        else if(recognizeAlgorithm==RecognizeAlgorithm::FisherFace)
        {
            cvImage = fisher()->prepareForRecognition(image);
        }
        else if(recognizeAlgorithm==RecognizeAlgorithm::DNN)
        {
            cvImage = dnn()->prepareForRecognition(image);
        }
        else
        {
            qCCritical(DIGIKAM_FACESENGINE_LOG) << "No obvious recognize algorithm";
        }
        */
        //cvImage         = aligner()->align(cvImage);
        //TanTriggsPreprocessor preprocessor;
        //cvImage         = preprocessor.preprocess(cvImage);
        return cvImage;
    }
    catch (cv::Exception& e)
    {
        qCCritical(DIGIKAM_FACESENGINE_LOG) << "cv::Exception:" << e.what();
        return cv::Mat();
    }
    catch(...)
    {
        qCCritical(DIGIKAM_FACESENGINE_LOG) << "Default exception from OpenCV";
        return cv::Mat();
    }
}

cv::Mat RecognitionDatabase::Private::preprocessingChainRGB(const QImage& image)
{
    try
    {
        //cv::Mat cvImage = recognizer()->prepareForRecognition(image);
        cv::Mat cvImage;
        cvImage = eigen()->prepareForRecognition(image);
        /*
        if(recognizeAlgorithm==RecognizeAlgorithm::LBP)
        {
            cvImage = lbph()->prepareForRecognition(image);
        }
        else if(recognizeAlgorithm==RecognizeAlgorithm::EigenFace)
        {
            cvImage = eigen()->prepareForRecognition(image);
        }
        else if(recognizeAlgorithm==RecognizeAlgorithm::FisherFace)
        {
            cvImage = fisher()->prepareForRecognition(image);
        }
        else if(recognizeAlgorithm==RecognizeAlgorithm::DNN)
        {
            cvImage = dnn()->prepareForRecognition(image);
        }
        else
        {
            qCCritical(DIGIKAM_FACESENGINE_LOG) << "No obvious recognize algorithm";
        }
        */
        //cvImage         = aligner()->align(cvImage);
        //TanTriggsPreprocessor preprocessor;
        //cvImage         = preprocessor.preprocess(cvImage);
        return cvImage;
    }
    catch (cv::Exception& e)
    {
        qCCritical(DIGIKAM_FACESENGINE_LOG) << "cv::Exception:" << e.what();
        return cv::Mat();
    }
    catch(...)
    {
        qCCritical(DIGIKAM_FACESENGINE_LOG) << "Default exception from OpenCV";
        return cv::Mat();
    }
}

void RecognitionDatabase::activeFaceRecognizer(int algorithmType)
{
    d->recognizeAlgorithm = (Private::RecognizeAlgorithm)algorithmType;
}

QList<Identity> RecognitionDatabase::recognizeFaces(ImageListProvider* const images)
{
    if (!d || !d->dbAvailable)
    {
        return QList<Identity>();
    }

    QMutexLocker lock(&d->mutex);

    QList<Identity> result;

    for (; !images->atEnd(); images->proceed())
    {
        int id = -1;

        try
        {
            //id = d->recognizer()->recognize(d->preprocessingChain(images->image()));
            if(d->recognizeAlgorithm==Private::RecognizeAlgorithm::LBP)
            {
                id = d->lbph()->recognize(d->preprocessingChain(images->image()));
            }
            else if(d->recognizeAlgorithm==Private::RecognizeAlgorithm::EigenFace)
            {
                id = d->eigen()->recognize(d->preprocessingChain(images->image()));
            }
            else if(d->recognizeAlgorithm==Private::RecognizeAlgorithm::FisherFace)
            {
                id = d->fisher()->recognize(d->preprocessingChain(images->image()));
            }
            else if(d->recognizeAlgorithm==Private::RecognizeAlgorithm::DNN)
            {
                id = d->dnn()->recognize(d->preprocessingChainRGB(images->image()));
            }
        }
        catch (cv::Exception& e)
        {
            qCCritical(DIGIKAM_FACESENGINE_LOG) << "cv::Exception:" << e.what();
        }
        catch(...)
        {
            qCCritical(DIGIKAM_FACESENGINE_LOG) << "Default exception from OpenCV";
        }

        if (id == -1)
        {
            result << Identity();
        }
        else
        {
            result << d->identityCache.value(id);
        }
    }

    return result;
}

RecognitionDatabase::TrainingCostHint RecognitionDatabase::trainingCostHint() const
{
    return TrainingIsCheap;
}

void RecognitionDatabase::train(const Identity& identityToBeTrained, TrainingDataProvider* const data,
                                const QString& trainingContext)
{
    train(QList<Identity>() << identityToBeTrained, data, trainingContext);
}

/// Training where the train method takes one identity and one image
/*
template <class Recognizer>
static void trainSingle(Recognizer* const r, const Identity& identity, TrainingDataProvider* const data,
                        const QString& trainingContext, RecognitionDatabase::Private* const d)
{
    ImageListProvider* const images = data->newImages(identity);

    qCDebug(DIGIKAM_FACESENGINE_LOG) << "Training " << images->size() << " images for identity " << identity.id();

    for (; !images->atEnd(); images->proceed())
    {
        try
        {
            r->train(identity.id(), d->preprocessingChain(images->image()), trainingContext);
        }
        catch (cv::Exception& e)
        {
            qCCritical(DIGIKAM_FACESENGINE_LOG) << "cv::Exception:" << e.what();
        }
        catch(...)
        {
            qCCritical(DIGIKAM_FACESENGINE_LOG) << "Default exception from OpenCV";
        }
    }
}
*/

/// Training where the train method takes a list of identities and images,
/// and updating per-identity is non-inferior to updating all at once.
//template <class Recognizer>
static void trainIdentityBatchLBPH(OpenCVLBPHFaceRecognizer* const r, const QList<Identity>& identitiesToBeTrained,
                               TrainingDataProvider* const data, const QString& trainingContext,
                               RecognitionDatabase::Private* const d)
{
    foreach (const Identity& identity, identitiesToBeTrained)
    {
        std::vector<int>     labels;
        std::vector<cv::Mat> images;

        ImageListProvider* const imageList = data->newImages(identity);
        images.reserve(imageList->size());

        for (; !imageList->atEnd(); imageList->proceed())
        {
            try
            {
                cv::Mat cvImage = d->preprocessingChain(imageList->image());
                cv::Mat cvImage_rgb = d->preprocessingChainRGB(imageList->image());

                labels.push_back(identity.id());
                images.push_back(cvImage);
            }
            catch (cv::Exception& e)
            {
                qCCritical(DIGIKAM_FACESENGINE_LOG) << "cv::Exception preparing image for LBPH:" << e.what();
            }
            catch(...)
            {
                qCCritical(DIGIKAM_FACESENGINE_LOG) << "Default exception from OpenCV";
            }
        }

        qCDebug(DIGIKAM_FACESENGINE_LOG) << "Training " << images.size() << " images for identity " << identity.id();

        try
        {
            r->train(images, labels, trainingContext);
        }
        catch (cv::Exception& e)
        {
            qCCritical(DIGIKAM_FACESENGINE_LOG) << "cv::Exception training Recognizer:" << e.what();
        }
        catch(...)
        {
            qCCritical(DIGIKAM_FACESENGINE_LOG) << "Default exception from OpenCV";
        }
    }
}

static void trainIdentityBatchEIGEN(OpenCVEIGENFaceRecognizer* const r, const QList<Identity>& identitiesToBeTrained,
                               TrainingDataProvider* const data, const QString& trainingContext,
                               RecognitionDatabase::Private* const d)
{
    foreach (const Identity& identity, identitiesToBeTrained)
    {
        std::vector<int>     labels;
        std::vector<cv::Mat> images;
        std::vector<cv::Mat> images_rgb;

        ImageListProvider* const imageList = data->newImages(identity);
        images.reserve(imageList->size());

        for (; !imageList->atEnd(); imageList->proceed())
        {
            try
            {
                cv::Mat cvImage = d->preprocessingChain(imageList->image());
                cv::Mat cvImage_rgb = d->preprocessingChainRGB(imageList->image());

                labels.push_back(identity.id());
                images.push_back(cvImage);
                images_rgb.push_back(cvImage_rgb);
            }
            catch (cv::Exception& e)
            {
                qCCritical(DIGIKAM_FACESENGINE_LOG) << "cv::Exception preparing image for LBPH:" << e.what();
            }
            catch(...)
            {
                qCCritical(DIGIKAM_FACESENGINE_LOG) << "Default exception from OpenCV";
            }
        }

        qCDebug(DIGIKAM_FACESENGINE_LOG) << "Training " << images.size() << " images for identity " << identity.id();

        try
        {
            r->train(images, labels, trainingContext, images_rgb);
        }
        catch (cv::Exception& e)
        {
            qCCritical(DIGIKAM_FACESENGINE_LOG) << "cv::Exception training Recognizer:" << e.what();
        }
        catch(...)
        {
            qCCritical(DIGIKAM_FACESENGINE_LOG) << "Default exception from OpenCV";
        }
    }
}

void RecognitionDatabase::Private::train(OpenCVLBPHFaceRecognizer* const r, const QList<Identity>& identitiesToBeTrained,
                                         TrainingDataProvider* const data, const QString& trainingContext)
{
    trainIdentityBatchLBPH(r, identitiesToBeTrained, data, trainingContext, this);
}

void RecognitionDatabase::Private::train(OpenCVEIGENFaceRecognizer* const r, const QList<Identity>& identitiesToBeTrained,
                                         TrainingDataProvider* const data, const QString& trainingContext)
{
    qCDebug(DIGIKAM_FACESENGINE_LOG) << "Training using opencv eigenfaces";
    trainIdentityBatchEIGEN(r, identitiesToBeTrained, data, trainingContext, this);
}

void RecognitionDatabase::train(const QList<Identity>& identitiesToBeTrained, TrainingDataProvider* const data,
                                const QString& trainingContext)
{
    if (!d || !d->dbAvailable)
    {
        return;
    }

    QMutexLocker lock(&d->mutex);

    //d->train(d->recognizer(), identitiesToBeTrained, data, trainingContext);
    d->train(d->lbph(), identitiesToBeTrained, data, trainingContext);
    d->train(d->eigen(), identitiesToBeTrained, data, trainingContext);
}


void RecognitionDatabase::train(const Identity& identityToBeTrained, const QImage& image,
                                const QString& trainingContext)
{
    SimpleTrainingDataProvider* const data = new SimpleTrainingDataProvider(identityToBeTrained,
                                                                            QList<QImage>() << image);
    train(identityToBeTrained, data, trainingContext);
    delete data;
}

void RecognitionDatabase::train(const Identity& identityToBeTrained, const QList<QImage>& images,
                                const QString& trainingContext)
{
    SimpleTrainingDataProvider* const data = new SimpleTrainingDataProvider(identityToBeTrained, images);
    train(identityToBeTrained, data, trainingContext);
    delete data;
}

void RecognitionDatabase::Private::clear(OpenCVLBPHFaceRecognizer* const, const QList<int>& idsToClear, const QString& trainingContext)
{
    // force later reload
    delete opencvlbph;
    opencvlbph = 0;

    if (idsToClear.isEmpty())
    {
        FaceDbAccess().db()->clearLBPHTraining(trainingContext);
    }
    else
    {
        FaceDbAccess().db()->clearLBPHTraining(idsToClear, trainingContext);
    }
}

void RecognitionDatabase::Private::clear(OpenCVEIGENFaceRecognizer* const, const QList<int>& idsToClear, const QString& trainingContext)
{
    // force later reload
    delete opencveigen;
    opencveigen = 0;

    if (idsToClear.isEmpty())
    {
        FaceDbAccess().db()->clearEIGENTraining(trainingContext);
    }
    else
    {
        FaceDbAccess().db()->clearEIGENTraining(idsToClear, trainingContext);
    }
}

void RecognitionDatabase::Private::clear(OpenCVFISHERFaceRecognizer* const, const QList<int>& idsToClear, const QString& trainingContext)
{
    // force later reload
    delete opencvfisher;
    opencvfisher = 0;

    if (idsToClear.isEmpty())
    {
        FaceDbAccess().db()->clearEIGENTraining(trainingContext);
    }
    else
    {
        FaceDbAccess().db()->clearEIGENTraining(idsToClear, trainingContext);
    }
}

void RecognitionDatabase::Private::clear(OpenCVDNNFaceRecognizer* const,
                                         const QList<int>& idsToClear,
                                         const QString& trainingContext)
{
    // force later reload
    delete opencvdnn;
    opencvdnn = 0;

    if (idsToClear.isEmpty())
    {
        FaceDbAccess().db()->clearEIGENTraining(trainingContext);
    }
    else
    {
        FaceDbAccess().db()->clearEIGENTraining(idsToClear, trainingContext);
    }
}

void RecognitionDatabase::clearAllTraining(const QString& trainingContext)
{
    if (!d || !d->dbAvailable)
    {
        return;
    }

    QMutexLocker lock(&d->mutex);
    //d->clear(d->recognizer(), QList<int>(), trainingContext);
    d->clear(d->lbph(), QList<int>(), trainingContext);
    d->clear(d->eigen(), QList<int>(), trainingContext);
    d->clear(d->fisher(), QList<int>(), trainingContext);
    d->clear(d->dnn(), QList<int>(), trainingContext);
}

void RecognitionDatabase::clearTraining(const QList<Identity>& identitiesToClean, const QString& trainingContext)
{
    if (!d || !d->dbAvailable || identitiesToClean.isEmpty())
    {
        return;
    }

    QMutexLocker lock(&d->mutex);
    QList<int>   ids;

    foreach (const Identity& id, identitiesToClean)
    {
        ids << id.id();
    }

    d->clear(d->recognizer(), ids, trainingContext);
}

void RecognitionDatabase::deleteIdentity(const Identity& identityToBeDeleted)
{
    if (!d || !d->dbAvailable || identityToBeDeleted.isNull())
    {
        return;
    }

    QMutexLocker lock(&d->mutex);

    FaceDbAccess().db()->deleteIdentity(identityToBeDeleted.id());
    d->identityCache.remove(identityToBeDeleted.id());
}

bool RecognitionDatabase::integrityCheck()
{
    if (!d || !d->dbAvailable)
    {
        return false;
    }

    QMutexLocker lock(&d->mutex);

    return FaceDbAccess().db()->integrityCheck();
}

void RecognitionDatabase::vacuum()
{
    if (!d || !d->dbAvailable)
    {
        return;
    }

    QMutexLocker lock(&d->mutex);

    return FaceDbAccess().db()->vacuum();
}

} // namespace Digikam
