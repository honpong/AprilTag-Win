from enum import Enum


class ProductDefinition(Enum):
    """
    Valid type supported by Product Manager.
    """
    RASTER = 'raster',
    RASTER_DEM = 'rasterDEM',
    RASTER_ORTHO = 'rasterOrtho',
    VECTOR = 'vector',
    REPORT = 'report'
    GCP = 'gcp',
    PCL = 'pcl',
    MESH = 'mesh',
    VECTOR_TILES = 'vectortiles',
    RASTER_TILES = 'rastertiles',
    VIDEO = 'video',
    ATTACHMENT = 'attachment',
    DXOBJECT_COLLECTION = 'dxobjectCollection'
