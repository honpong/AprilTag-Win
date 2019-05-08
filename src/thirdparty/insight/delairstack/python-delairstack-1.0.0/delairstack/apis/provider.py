import json
import logging

from ..core.connection.connection import Connection
from ..core.utils.utils import md5
from ..core.utils.utils import sanitize_dict


class Provider(object):
    SEARCH_PATH = 'search'
    _root_path = ''

    def __init__(self, connection: Connection):
        self._connection = connection

    def search(self, path, query):
        """
            Search objects.
        """

        content = self._connection.post(
                path='{root_path}/{path}/{search_path}'.format(root_path=self._root_path,
                                                               path=path,
                                                               search_path=self.SEARCH_PATH),
                headers={'Cache-Control': 'no-cache', 'Content-Type': 'application/json'},
                data=json.dumps(query),
                as_json=True)

        return content

    def all(self, path):
        content = self._connection.get(
                path='{root_path}/{path}'.format(root_path=self._root_path, path=path),
                headers={'Cache-Control': 'no-cache'},
                as_json=True)

        return content

    def get(self, path):
        content = self._connection.get(
                path='{root_path}/{path}'.format(root_path=self._root_path, path=path),
                headers={'Cache-Control': 'no-cache'},
                as_json=True)

        return content

    def lazy_get(self, path):
        response = self._connection.lazy_get(
                path='{root_path}/{path}'.format(root_path=self._root_path, path=path),
                headers={'Cache-Control': 'no-cache'})

        return response

    def post(self, path, data, as_json=True):
        """
        create the object corresponding to desc
        """

        headers = {'Cache-Control': 'no-cache', 'Content-Type': 'application/json'}

        content = self._connection.post(
                path='{root_path}/{path}'.format(root_path=self._root_path, path=path),
                headers=headers, data=json.dumps(data),
                as_json=as_json)

        # Build and return new object from properties
        return content

    def put(self, path, data, sanitize=True):
        """update an object based on the update property

        If partial_description is not defined, will update the whole
        description of the object(provided by dx_object.desc) e.g:
        update_url="name", partial_description={'name'='toto'} will
        update the name of the object sanitize (default: True): remove
        special character before sending the request

        """

        data_to_send = data

        if sanitize is True:
            data_to_send = sanitize_dict(data)

        headers = {'Cache-Control': 'no-cache', 'Content-Type': 'application/json'}

        # set the properties into DB
        content = self._connection.put(
                path='{root_path}/{path}'.format(root_path=self._root_path, path=path),
                headers=headers,
                data=json.dumps(data_to_send),
                as_json=True)

        # return JSON content
        return content

    def delete(self, path):
        """
            Delete object.
        """

        headers = {'Cache-Control': 'no-cache', 'Content-Type': 'application/json'}

        # set the properties into DB
        content = self._connection.delete(
                path='{root_path}/{path}'.format(root_path=self._root_path, path=path),
                headers=headers,
                as_json=True)

        # return JSON content
        return content


class AuthAPI(Provider):
    _root_path = 'dxauth'


class TileServerAPI(Provider):
    _root_path = 'tileserver'


class DataStoreAPI(Provider):
    _root_path = 'dxds'
    logger = logging.getLogger(__name__)

    def upload_file(self, *, src_file_path, path):
        # defines a custom route to upload file. This is not supported by the multiUpload
        headers = {
            'Cache-Control': 'no-cache', 'Content-Type': 'application/octet-stream', 'Content-MD5': md5(
                    src_file_path)
            }

        path = '{root_path}/{path}'.format(root_path=self._root_path, path=path)

        # search file content and write it
        with open(src_file_path, 'rb') as file_reader:
            data = file_reader.read()

            # streaming upload
            # raise an exception if something get wrong
            self._connection.put(path=path, data=data, headers=headers, as_json=False)


class ProjectManagerAPI(Provider):
    _root_path = 'dxpm'


class TaskManagerAPI(Provider):
    _root_path = 'dxtm'
