#include <mpi.h>
#include "parameterwrapper.h"
#include "DebugOut/debug.h"
#include "GLGridLeaper.h"

DECLARE_CHANNEL(params);
DECLARE_CHANNEL(sync);

ParameterWrapper* ParamFactory::createFrom(NetDSCommandCode cmd, int socket) {
    switch(cmd) {
    case nds_OPEN:
        return new OpenParams(socket);
        break;
    case nds_CLOSE:
        return new CloseParams(socket);
        break;
    case nds_BRICK:
        return new BrickParams(socket);
        break;
    case nds_LIST_FILES:
        return new ListFilesParams(cmd);
        break;
    case nds_SHUTDOWN:
        return new ShutdownParams(cmd);
        break;
    case nds_ROTATION:
        return new RotateParams(cmd);
        break;
    case nds_BATCHSIZE:
        return new BatchSizeParams(socket);
        break;
    default:
        printf("Unknown command received.\n");
        break;
    }

    return NULL;
}


/*#################################*/
/*#######   Constructors    #######*/
/*#################################*/

ParameterWrapper::ParameterWrapper(NetDSCommandCode code)
    :code(code)
{}

ParameterWrapper::~ParameterWrapper() {}

OpenParams::OpenParams(int socket)
    :ParameterWrapper(nds_OPEN){
    if (socket != -1)
        initFromSocket(socket);
}

CloseParams::CloseParams(int socket)
    :ParameterWrapper(nds_CLOSE){
    if (socket != -1)
        initFromSocket(socket);
}

BatchSizeParams::BatchSizeParams(int socket)
    :ParameterWrapper(nds_BATCHSIZE){
    if (socket != -1)
        initFromSocket(socket);
}

BrickParams::BrickParams(int socket)
    :ParameterWrapper(nds_BRICK){
    if (socket != -1)
        initFromSocket(socket);
}

RotateParams::RotateParams(int socket)
    :ParameterWrapper(nds_ROTATION) {
    if (socket != -1)
        initFromSocket(socket);
}

SimpleParams::SimpleParams(NetDSCommandCode code)
    :ParameterWrapper(code) {}

ListFilesParams::ListFilesParams(NetDSCommandCode code)
    :SimpleParams(code) {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if(rank == 0) {
        TRACE(params, "LIST");
    }
}

ShutdownParams::ShutdownParams(NetDSCommandCode code)
    :SimpleParams(code) {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if(rank == 0) {
        TRACE(params, "SHUTDOWN");
    }
}


/*#################################*/
/*#######   Initialisators  #######*/
/*#################################*/

void OpenParams::initFromSocket(int socket) {
    ru16(socket, &len);

    filename = new char[len];
    readFromSocket(socket, filename, len*sizeof(char));
    TRACE(params, "OPEN (%d) %s", len, filename);
}

void CloseParams::initFromSocket(int socket) {
    ru16(socket, &len);

    filename = new char[len];
    readFromSocket(socket, filename, len*sizeof(char));
    TRACE(params, "CLOSE (%d) %s", len, filename);
}

void BatchSizeParams::initFromSocket(int socket) {
    rsizet(socket, &newBatchSize);

    TRACE(params, "BATCHSIZE %zu", newBatchSize);
}

void RotateParams::initFromSocket(int socket) {
    rf32v(socket, &matrix, &matSize);
    ru8(socket, &type);
    TRACE(params, "ROTATE");
}

void BrickParams::initFromSocket(int socket) {
    ru8(socket, &type);
    ru32(socket, &lod);
    ru32(socket, &bidx);
    TRACE(params, "BRICK lod=%u, bidx=%u", lod, bidx);
}

void SimpleParams::initFromSocket(int socket) {(void)socket;}


/*#################################*/
/*######  Writing to socket  ######*/
/*#################################*/

void OpenParams::writeToSocket(int socket) {
    wru8(socket, code);
    wru16(socket, len);
    wr(socket, filename, len);
}

void CloseParams::writeToSocket(int socket) {
    wru8(socket, code);
    wru16(socket, len);
    wr(socket, filename, len);
}

void BatchSizeParams::writeToSocket(int socket) {
    wru8(socket, code);
    wrsizet(socket, newBatchSize);
}

void RotateParams::writeToSocket(int socket) {
    wru8(socket, code);
    wrf32v(socket, matrix, 16);
}

void BrickParams::writeToSocket(int socket) {
    wru8(socket, code);
    wru32(socket, lod);
    wru32(socket, bidx);
}

void SimpleParams::writeToSocket(int socket) {
    wru8(socket, code);
}


/*#################################*/
/*######     MPI Syncing     ######*/
/*#################################*/

void OpenParams::mpi_sync(int rank, int srcRank) {
    MPI_Bcast(&len, 1, MPI_UNSIGNED_SHORT, srcRank, MPI_COMM_WORLD);

    if (rank != srcRank)
        filename = new char[len];
    MPI_Bcast(&filename[0], len, MPI_UNSIGNED_CHAR, srcRank, MPI_COMM_WORLD);

    if(rank != srcRank) {
        TRACE(sync, "proc %d open received %s (%d)", rank, filename, len);
    }
}

void CloseParams::mpi_sync(int rank, int srcRank) {
    MPI_Bcast(&len, 1, MPI_UNSIGNED_SHORT, srcRank, MPI_COMM_WORLD);
    if (rank != srcRank)
        filename = new char[len];
    MPI_Bcast(&filename[0], len, MPI_CHAR, srcRank, MPI_COMM_WORLD);

    if(rank != srcRank) {
        TRACE(sync, "proc %d close received %s (%d)", rank, filename, len);
    }
}

void BatchSizeParams::mpi_sync(int rank, int srcRank) {
    uint32_t tmp = (uint32_t)newBatchSize;
    MPI_Bcast(&tmp, 1, MPI_UNSIGNED, srcRank, MPI_COMM_WORLD);
    newBatchSize = (size_t)tmp;

    if(rank != srcRank) {
        TRACE(sync, "proc %d setBatchSize received with %zu", rank, newBatchSize);
    }
}

void RotateParams::mpi_sync(int rank, int srcRank) {
    MPI_Bcast(&matrix[0], 16, MPI_FLOAT, srcRank, MPI_COMM_WORLD);

    if(rank != srcRank) {
        TRACE(sync, "proc %d rotate received", rank);
    }
}

void BrickParams::mpi_sync(int rank, int srcRank) {
    MPI_Bcast(&type, 1, MPI_UNSIGNED_CHAR, srcRank, MPI_COMM_WORLD);
    MPI_Bcast(&lod, 1, MPI_UNSIGNED, srcRank, MPI_COMM_WORLD);
    MPI_Bcast(&bidx, 1, MPI_UNSIGNED, srcRank, MPI_COMM_WORLD);

    if(rank != srcRank) {
        TRACE(sync, "proc %d brick received: lod %u & bidx: %u", rank, lod,
              bidx);
    }
}

void SimpleParams::mpi_sync(int rank, int srcRank) {(void)rank; (void)srcRank;}


/*#################################*/
/*######     Executing       ######*/
/*#################################*/

void OpenParams::perform(int socket, CallPerformer* object) {
    object->openFile(filename);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if(rank != 0)
        return; //mpi currently not supported

    //send LODs
    size_t lodCount = object->ds->GetLODLevelCount();
    wrsizet(socket, lodCount);

    //Send layouts
    size_t layoutsCount = lodCount * 3;
    uint32_t layouts[layoutsCount];
    for(size_t lod=0; lod < lodCount; ++lod) {
        UINTVECTOR3 layout = object->ds->GetBrickLayout(lod, 0);
        layouts[lod * 3 + 0] = layout.x;
        layouts[lod * 3 + 1] = layout.y;
        layouts[lod * 3 + 2] = layout.z;
    }
    wru32v(socket, &layouts[0], layoutsCount);

    //write total count of bricks out
    size_t brickCount = object->ds->GetTotalBrickCount();
    wrsizet(socket, brickCount);

    //We need the brick Keys
    size_t lods[brickCount];
    size_t idxs[brickCount];
    //And the brickMD
    float md_centers[brickCount * 3];
    float md_extents[brickCount * 3];
    uint32_t md_n_voxels[brickCount * 3];

    //Actually read the data
    size_t i = 0;
    for(auto brick = object->ds->BricksBegin(); brick != object->ds->BricksEnd(); brick++, i++) {
        tuvok::BrickKey key    = brick->first;
        tuvok::BrickMD md      = brick->second;

        lods[i] = std::get<1>(key);
        idxs[i] = std::get<2>(key);

        md_centers[i*3 + 0] = md.center.x;
        md_centers[i*3 + 1] = md.center.y;
        md_centers[i*3 + 2] = md.center.z;

        md_extents[i*3 + 0] = md.extents.x;
        md_extents[i*3 + 1] = md.extents.y;
        md_extents[i*3 + 2] = md.extents.z;

        md_n_voxels[i*3 + 0] = md.n_voxels.x;
        md_n_voxels[i*3 + 1] = md.n_voxels.y;
        md_n_voxels[i*3 + 2] = md.n_voxels.z;
    }

    wrsizetv_d(socket, &lods[0], brickCount);
    wrsizetv_d(socket, &idxs[0], brickCount);
    wrf32v_d(socket, &md_centers[0], brickCount * 3);
    wrf32v_d(socket, &md_extents[0], brickCount * 3);
    wru32v_d(socket, &md_n_voxels[0], brickCount * 3);
}

void CloseParams::perform(int socket, CallPerformer* object) {
    object->closeFile(filename);
    (void)socket; //currently no answer
}

void BatchSizeParams::perform(int socket, CallPerformer *object) {
    object->maxBatchSize = newBatchSize;
    (void)socket;
}

void RotateParams::perform(int socket, CallPerformer *object) {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if(rank != 0)
        return; //mpi currently not supported

    //renders the scene
    object->rotate(matrix);

    //Retrieve a list of bricks that need to be send to the client
    const tuvok::GLGridLeaper* glren = dynamic_cast<tuvok::GLGridLeaper*>(object->renderer);
    assert(glren && "not a grid leaper?  wrong renderer in us?");
    const std::vector<UINTVECTOR4> hash = glren->GetNeededBricks();
    const tuvok::LinearIndexDataset& ds = dynamic_cast<const tuvok::LinearIndexDataset&>(*object->ds);

    size_t totalBrickCount = hash.size();
    std::vector<tuvok::BrickKey> allKeys(totalBrickCount);
    for(UINTVECTOR4 b : hash) {
        allKeys.push_back(ds.IndexFrom4D(b, 0));
    }

    NetDataType dType = (NetDataType)type;
    if(dType == N_UINT8)
        startBrickSendLoop<uint8_t>(socket, object, allKeys);
    else if(dType == N_UINT16)
        startBrickSendLoop<uint16_t>(socket, object, allKeys);
    else if(dType == N_UINT32)
        startBrickSendLoop<uint32_t>(socket, object, allKeys);
}

void BrickParams::perform(int socket, CallPerformer* object) {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if(rank == 0) {
        NetDataType dType = (NetDataType)type;

        if(dType == N_UINT8)
            internal_brickPerform<uint8_t>(socket, object);
        else if(dType == N_UINT16)
            internal_brickPerform<uint16_t>(socket, object);
        else if(dType == N_UINT32)
            internal_brickPerform<uint32_t>(socket, object);
    }
}

void ListFilesParams::perform(int socket, CallPerformer* object) {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if(rank == 0) {
        vector<std::string> filenames = object->listFiles();

        wru16(socket, (uint16_t)filenames.size());
        for(std::string name : filenames) {
            const char* cstr = name.c_str();
            wrCStr(socket, cstr);
        }
    }
}

void ShutdownParams::perform(int socket, CallPerformer* object) {
    //Not necessary
    (void)socket; //currently no answer
    (void)object;
}