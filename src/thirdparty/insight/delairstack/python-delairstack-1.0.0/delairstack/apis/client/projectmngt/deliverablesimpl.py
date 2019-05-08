"""Module dedicated to the deliverable resource management

"""
import logging

from .dxobjectsimpl import DXObjectsImpl
from ...provider import DataStoreAPI, ProjectManagerAPI
from ....core.errors import (ArgumentOperationError,
                             UnsupportedOperationError)
from ....core.resources.products import ProductDefinition
from ....core.resources.projectmngt.deliverables import Deliverables
from ....core.resources.resource import Resource
from ....core.utils.filehelper import FileHelper


class DeliverablesImpl(Deliverables):
    """This class represents an accessor to handle deliverable objects and actions

    """
    _template = {}
    _hidden = ['name']
    _immutable = ['dxobject', 'object', 'orderer_user_id', 'product_id', 'product_type',
                  'status', 'subtype']

    logger = logging.getLogger(__name__)

    def __init__(self, project_manager_api: ProjectManagerAPI, data_store_api: DataStoreAPI, **kwargs):
        super().__init__(provider=project_manager_api)
        self._data_store_api = data_store_api

    def create(self, *, mission: Resource = None,
               project: Resource, name: str, product_type: ProductDefinition):
        """Create a Deliverable which will be associated to the project.

        Args:
            mission (Resource): the associated mission, optional
            project (Resource): the associated project
            name (str): the name of the deliverable
            product_type (ProductDefinition): the product type

        Returns:
            Resource: the object representing a deliverable

        """

        self.logger.debug('Create deliverable resource')
        # Merge the template with kwargs
        # kwargs will override existing properties contained into template
        data = {'type': product_type.value[0],
                'subtype': product_type.value[0],
                'name': name,
                'displayName': name,
                **self._template}

        if project is None:
            raise ArgumentOperationError('Expecting project argument')

        data.update({'project_id': project.id})
        if mission is not None:
            data.update({'mission_id': mission.id})

        content = self._provider.post(path=self._name, data=data)

        # associates DXObject resource to the deliverable one
        deliverable = content['deliverable']

        deliverable['dxobject'] = content['dxobject']['_id']

        return Resource(id=deliverable['id'], desc=deliverable, manager=self)

    def upload(self, *, deliverable: Resource, files: [str]):
        """Upload list of files into a deliverable.

        Args:
            deliverable (Resource): the deliverable
            files [str]: the list of files to upload. It must contains either a list of relative or/and absolute path

        """
        # gets type of dxobject
        # special case for photos because it uses different routes
        dxobject_resource_manager = DXObjectsImpl(project_manager_api=self._provider,
                                                  data_store_api=self._data_store_api)

        dxobject = dxobject_resource_manager.get(id=deliverable.dxobject)

        for file in files:
            basename = FileHelper.get_base_name_without_extension(file)

            # add file metadata to dxobject
            new_files = {}
            new_files[basename] = {'url': basename, 'status': 'uploading', 'ext': FileHelper.get_file_extension(file)}

            dxobject.files = new_files
            # update DXObject
            dxobject_resource_manager.update(resource=dxobject)

        for file in files:
            # create a new empty Resource object to be accepted as input of the upload function
            dxobject_resource_manager.upload(dxobject=dxobject, file_src_path=file)

    def search(self, **kwargs):
        raise UnsupportedOperationError()
