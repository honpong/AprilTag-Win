"""Configuration.

"""

import json
import logging

from pkg_resources import resource_filename

from .utils.utils import dict_merge

__all__ = ('ConnectionConfig', 'ResourcesConfig')


class _Config(object):
    logger = logging.getLogger(__name__)

    def __init__(self, defaults=None, user_path=None, **kwargs):
        default_conf = defaults or {}
        user_conf = {}

        if user_path:
            self.logger.info('Loading user configuration {}'.format(user_path))
            user_conf = self._read_user_conf(user_path)

        conf = dict_merge({}, default_conf)
        conf = dict_merge(conf, user_conf)

        # reads user parameters
        conf = dict_merge(conf, kwargs)

        for name, val in conf.items():
            setattr(self, name, val)

    @staticmethod
    def _read_user_conf(file_path):
        with open(file_path) as f:
            return json.load(f)


class ConnectionConfig(_Config):
    def __init__(self, file_path=None, **kwargs):
        defaults = json.load(open(resource_filename(__name__,
                                                    'config-connection.json')))
        super().__init__(defaults=defaults, user_path=file_path, **kwargs)


class ResourcesConfig(_Config):
    def __init__(self, file_path=None):
        defaults = json.load(open(resource_filename(__name__,
                                                    'config-resources.json')))
        super().__init__(defaults=defaults, user_path=file_path)
