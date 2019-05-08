"""Module dedicated to the photos resource management.

"""
import logging
import os

from delairstack.apis.provider import DataStoreAPI, ProjectManagerAPI
from delairstack.core.errors import DownloadError
from delairstack.core.resources.projectmngt.photos import Photos
from delairstack.core.resources.resource import Resource
from ....core.errors import (UnsupportedOperationError)
from ....core.utils.filehelper import FileHelper


class PhotosImpl(Photos):
    """This class represents an accessor to handle photo objects and actions.

    """
    _template = {}
    _hidden = ['RTC', 'annotations_count', 'calibration', 'interest', 'location',
               'metadata', 'pan_deg', 'tilt_deg', 'sharpened', 'size']
    _immutable = ['camera', 'camera_id', 'created', 'flight', 'flight_id', 'modified',
                  'phi', 'theta', 'psi', 'seq', 'shutter', 'types', 'user']

    logger = logging.getLogger(__name__)

    def __init__(self, project_manager_api: ProjectManagerAPI, data_store_api: DataStoreAPI, **kwargs):
        super().__init__(provider=project_manager_api, **kwargs)
        self._data_store_api = data_store_api

    def upload(self, photos: [Resource], mission: Resource, files):
        raise UnsupportedOperationError()

    def download(self, photo: Resource, destination_directory_path: str):
        """Download a file attached to the dxobject resource. The final destination name will be as
        <destination_directory_path>/<id>.<type>

        Args:
            photo (Resource): the DXObject
            destination_directory_path (str): the destination directory

        """
        # check status
        if not photo.types or not len(photo.types) > 0 or not photo.types[0]['status'] == 'available':
            raise DownloadError("The photo is not available yet")

        type = photo.types[0]['type']

        remote_path = '{name}/{type}/{resource_id}'.format(
                name=self._name,
                type=type,
                resource_id=photo.id)

        self.logger.info('Downloading file {0} from datastore..'.format(remote_path))

        # downloads file from datastore
        response = self._data_store_api.lazy_get(remote_path)

        # writes new downloaded file into destination directory (local disk)
        output_name = '{name}.{type}'.format(name=photo.types[0]['photo_id'], type=type)

        dst_file = os.path.join(destination_directory_path, output_name)
        self.logger.info('Writting file onto disk {0} ..'.format(dst_file))
        FileHelper.write_as_file(dst_file, response)

    def create(self, **kwargs):
        raise UnsupportedOperationError()

    def update(self, **kwargs):
        raise UnsupportedOperationError()

    def delete(self, **kwargs):
        raise UnsupportedOperationError()

    def search(self, *, mission: Resource):
        """Get all photos contained in a mission.

        Args:
            mission (Resource): the mission

        Returns:
             [Resource]: A list of photos matching with the search criteria

        """
        query = {
            'flights_id': [mission.flights[0]]
            }
        return super().search(query=query)
