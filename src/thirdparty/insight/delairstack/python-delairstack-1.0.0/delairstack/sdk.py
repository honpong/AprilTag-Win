"""Base SDK module to get the resources.

"""
import logging

from .apis.provider import (ProjectManagerAPI, DataStoreAPI, TaskManagerAPI)
from .core.config import ConnectionConfig, ResourcesConfig
from .core.connection.connection import Connection
from .core.connection.credentials import ClientCredentials
from .core.connection.credentials import UserCredentials
from .core.errors import (ConfigError, UnsupportedResourceError)
from .core.utils.utils import new_instance


class DelairStackSDK(object):
    """Class providing a single resource accessor to get the resource manager objects.

    """
    # log default parameters
    logger = logging.getLogger(__name__)

    def __init__(self, config_path=None, *, resources_config_path=None):
        self.logger.info('Initializing SDK')
        connection_config = ConnectionConfig(config_path)

        self.__resources_config = ResourcesConfig(resources_config_path)

        credentials = None
        if getattr(connection_config, 'client_id', None):
            self.logger.debug('Using APIs client credentials')
            credentials = ClientCredentials(connection_config.client_id, connection_config.secret)
        elif getattr(connection_config, 'user', None):
            credentials = UserCredentials(connection_config.user, connection_config.password)
            self.logger.debug('Using user credentials')
        else:
            self.logger.error('Missing credentials')
            raise ConfigError('Credentials are required')

        # create connection
        self.logger.info('Initializing connection')
        try:
            _conn_opts = {
                'base_url': connection_config.url,
                'disable_ssl_certificate': connection_config.connection['disable_ssl_certificate'],
                'credentials': credentials,
                'max_retries': connection_config.connection['max_retries']
                }
            self.__connection = Connection(**_conn_opts)
        except Exception as exception:
            msg = 'Cannot establish a connection: {0}'.format(exception)
            self.logger.error(msg)
            raise exception

        api_args = {'connection': self.__connection}
        self.__config_apis = {
            'project_manager_api': ProjectManagerAPI(**api_args),
            'data_store_api': DataStoreAPI(**api_args),
            'task_manager_api': TaskManagerAPI(**api_args)
            }

    def resources(self, resource_name: str):
        """Get a resource manager corresponding to the name.

        Args:
            resource_name: the name of the project manager to
               instantiate:'projects'|'annotations'|'deliverables'|'dxobjects'|'missions'

        Returns: ResourceManagerBase

        """
        self.logger.debug('Loading resource {0}'.format(resource_name))
        if resource_name in self.__resources_config.resources:
            class_info = self.__resources_config.resources[resource_name]
            try:
                # using multiple arguments means the class is in charge of taking whatever it needs in the kwargs
                return new_instance(class_info['module'], class_info['class'], **self.__config_apis)
            except ConfigError as ex:
                msg = 'The resource {0} cannot be loaded {1}'.format(resource_name, ex)
                self.logger.error(msg)
                raise ex

        else:
            self.logger.error('The resource is not supported')
            raise UnsupportedResourceError(resource_name)
        pass

    def close(self):
        """Close connections stream if any
        """

        if self.__connection:
            self.__connection.close()
