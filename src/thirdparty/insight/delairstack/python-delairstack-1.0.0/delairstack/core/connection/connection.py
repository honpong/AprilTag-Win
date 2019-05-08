import json
import logging
from urllib.parse import urljoin

import urllib3

from .abstract_connection import AbstractConnection
from .async_connection import AsyncConnection
from ..errors import ResponseError


class Connection(AbstractConnection):
    logger = logging.getLogger(__name__)

    def __init__(self, base_url=None, disable_ssl_certificate=False, credentials=None, max_retries=5):
        super().__init__(base_url=base_url, disable_ssl_certificate=disable_ssl_certificate, credentials=credentials,
                         max_retries=max_retries)
        cert_reqs = 'CERT_NONE'

        if not disable_ssl_certificate:
            cert_reqs = 'CERT_REQUIRED'
        else:
            urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

        self._http = urllib3.PoolManager(cert_reqs=cert_reqs)

        # creates an asynchronous connection with an existing token manager
        self._async_connection = AsyncConnection(
                base_url=base_url,
                disable_ssl_certificate=disable_ssl_certificate,
                token_manager=self._token_manager,
                max_retries=max_retries)

    @property
    def asynchronous(self):
        return self._async_connection

    def post(self, path, headers=None, data=None, timeout=30.0, as_json=False):
        """
            POST utility method
        """
        url = urljoin(self._base_url, path)
        resp = self._send_request(
                params={
                    'url': url,
                    'body': data or {},
                    'headers': headers or {},
                    'method': 'POST',
                    'timeout': timeout,
                    'retries': self._retry_without_auth
                    })
        return json.loads(resp.data.decode('utf-8')) if as_json else resp.data

    def get(self, path, headers=None, timeout=30.0, as_json=False):
        """
             GET utility method
        """
        url = urljoin(self._base_url, path)
        resp = self._send_request(
                params={
                    'url': url,
                    'headers': headers or {},
                    'method': 'GET',
                    'timeout': timeout,
                    'retries': self._retry_without_auth
                    })
        return json.loads(resp.data.decode('utf-8')) if as_json else resp.data

    def lazy_get(self, path, headers=None, timeout=30.0):
        """
             Lazy GET utility method: use stream to handle response content
         """
        url = urljoin(self._base_url, path)
        resp = self._send_request(
                params={
                    'url': url,
                    'headers': headers or {},
                    'method': 'GET',
                    'timeout': timeout,
                    'retries': self._retry_without_auth,
                    'preload_content': False
                    })
        return resp

    def put(self, path, headers=None, data=None, timeout=30.0, as_json=False):
        """
            PUT utility method
        """
        url = urljoin(self._base_url, path)
        resp = self._send_request(
                params={
                    'url': url,
                    'body': data or {},
                    'headers': headers or {},
                    'method': 'PUT',
                    'timeout': timeout,
                    'retries': self._retry_without_auth
                    })
        return json.loads(resp.data.decode('utf-8')) if as_json else resp.data

    def delete(self, path, headers=None, data=None, timeout=30.0, as_json=False):
        """
            DELETE utility method
        """
        url = urljoin(self._base_url, path)
        resp = self._send_request(
                params={
                    'url': url,
                    'body': data or {},
                    'headers': headers or {},
                    'method': 'DELETE',
                    'timeout': timeout,
                    'retries': self._retry_without_auth
                    })
        return json.loads(resp.data.decode('utf-8')) if as_json else resp.data

    def _send_request(self, params):
        self._add_authorization_maybe(params['headers'])

        self.logger.debug('Making {} request to {}'.format(params['method'],
                                                           params['url']))
        response = self._http.request(**params)

        # in case of expired token
        if response.status == 401:
            self.logger.debug('Got a 401 status')
            self._renew_token()
            self._add_authorization_maybe(params['headers'])
            params['retries'] = self._retry_with_auth

            self.logger.debug('Retrying to request using the new token..')
            response = self._http.request(**params)
        elif response.status != 200:
            raise ResponseError('{status}: {message}'.format(status=response.status,
                                                             message=response.data))
        return response

    def close(self):
        pass
