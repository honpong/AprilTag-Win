"""Module dedicated to the dxobject resource management.

"""
import logging
import os

from ....apis.provider import DataStoreAPI, ProjectManagerAPI
from ....core.connection.upload import MultipartUpload
from ....core.errors import (UnsupportedOperationError, FileError, ArgumentOperationError)
from ....core.resources.projectmngt.dxobjects import DXObjects
from ....core.resources.resources_manager_base import Resource
from ....core.utils.filehelper import FileHelper


class DXObjectsImpl(DXObjects):
    """This class represents an accessor to handle dxobject objects and actions.

    """
    _template = {}
    _hidden = ['childrens', 'objects', 'parents', 'siblings', 'storage_locations', 'workflow']
    _immutable = ['created', 'mission', 'modified', 'project', 'properties', 'user']

    logger = logging.getLogger(__name__)

    def __init__(self, project_manager_api: ProjectManagerAPI, data_store_api: DataStoreAPI, **kwargs):
        super().__init__(provider=project_manager_api, **kwargs)
        self._data_store_api = data_store_api

    def upload(self, *, dxobject: Resource = None, file_src_path: str = None):
        """Add a file to the DXObject and upload the file to the server.

        Args:
            dxobject (Resource): the DXObject
            file_src_path (str): the file source path to upload and to associate

        """

        # test if dest file exists. Raise os.error if the file does not exist or is inaccessible.
        if os.path.getsize(file_src_path) == 0:
            msg = 'The source file is empty {0}'.format(file_src_path)
            self.logger.error(msg)
            raise FileError(msg)

        self.logger.info('Uploading file {0} to datastore..'.format(file_src_path))

        multipart_upload = MultipartUpload(connection=self._data_store_api._connection,
                                           base_url=self._data_store_api._root_path)
        multipart_upload.send(src_path=file_src_path, oid=dxobject.id, ext=FileHelper.get_file_extension(file_src_path))

    def download(self, *, dxobject: Resource, destination_directory_path: str):
        """Download a file attached to the dxobject resource. The final destination name will be as
        <destination_directory_path>/<name>

        Args:
            dxobject (Resource): the DXObject
            destination_directory_path (str): the destination directory

        """
        results = {}

        for _, file in dxobject.files.items():

            # defines remote route
            remote_path = '{name}/{resource_id}'.format(name=self._name, resource_id=dxobject.id)

            # test if dest file exists
            if not os.path.isdir(destination_directory_path):
                raise FileError('The destination directory does not exist {0}'.format(destination_directory_path))

            # iterates over location, gets and write the file
            for location in file['storage_locations']:
                # Gets location url from metadata
                current_remote_file_path = '{0}/{1}'.format(remote_path, location['file_url'])
                self.logger.info('Downloading file {0} from datastore..'.format(current_remote_file_path))

                # downloads file from datastore
                response = self._data_store_api.lazy_get(current_remote_file_path)

                # writes new downloaded file into destination directory (local disk)
                dst_file = os.path.join(destination_directory_path, '{name}.{ext}'.format(name=location['file_url'],
                                                                                          ext=file['ext']))
                self.logger.info('Writting file onto disk {0} ..'.format(dst_file))
                FileHelper.write_as_file(dst_file, response)

                results[location['file_url']] = dst_file

        return results

    def create(self, **kwargs):
        raise UnsupportedOperationError()

    def get(self, *, deliverable: Resource = None, id: str = None):
        """Read a resource. If a deliverable object is passed as argument, this will return the dxobject contained
            into it. Otherwise will get object given its id.

            Args:
                id (str): id of the dxobject to search
                deliverable (Resource): the deliverable holding a DXObject

            Returns:
                Resource: an dxobject

            Raises:
                ArgumentOperationError: if the deliverable resource and the id are None
        """
        if deliverable is not None:
            return super().get(id=deliverable.dxobject)
        elif id is not None:
            return super().get(id=id)
        else:
            raise ArgumentOperationError("Either a deliverable resource or object id is necessary to search the object")

    def search(self, **kwargs):
        raise UnsupportedOperationError()

    def update(self, *, resource: Resource):
        """Update the dxobject.

        Args:
            resource (Resource): the dxobject

        Returns:
            Resource: a new DXObject

        """
        # check files changes

        if resource.check_attribute('files'):
            return self.__update_partial(resource, 'properties')
        else:
            return super().update(resource=resource)

    def __update_partial(self, resource: Resource, property_name: str):
        content = self._provider.put(
                path='{name}/{id}/{property_name}'.format(name=self._name,
                                                          id=resource.id,
                                                          property_name=property_name
                                                          ),
                data=resource._desc,
                sanitize=False)
        return Resource(
                id=content['_id'],
                desc=content,
                manager=self)
