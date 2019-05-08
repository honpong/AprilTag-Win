from ..resource import Resource
from ..resources_manager_base import ResourcesManagerBase


class Deliverables(ResourcesManagerBase):
    _name = 'deliverables'

    def upload(self, *, resource: Resource, files):
        """
        Updates DXObject metadata and upload file to the project manager.
        The project manager is in charge to write the file into the Data store
        The
        :return:


        if name is None:
            raise SDKException("A name id has to be defined")
        if product_id is None:
            raise SDKException("A product id has to be defined")
        elif files is None:
            raise SDKException("A list of files has to be defined")"""

        pass
