import concurrent.futures as cf
import logging

from requests.adapters import HTTPAdapter
from requests_futures.sessions import FuturesSession
from urllib3.util.retry import Retry

from .abstract_connection import AbstractConnection


class AsyncConnection(AbstractConnection):
    logger = logging.getLogger(__name__)

    def __init__(self, base_url=None, disable_ssl_certificate=False, token_manager=None, max_requests_workers=6,
                 max_retries=3):
        super().__init__(base_url=base_url, disable_ssl_certificate=disable_ssl_certificate,
                         token_manager=token_manager, max_retries=max_retries)

        executor = cf.ThreadPoolExecutor(max_workers=max_requests_workers)
        adapter_kwargs = {
            'pool_connections': max_requests_workers,
            'pool_maxsize': max_requests_workers,
            'max_retries': Retry(total=max_retries, backoff_factor=1,
                                 status_forcelist=[403, 405, 409, 413, 429, 500, 502, 503, 504],
                                 method_whitelist=frozenset(
                                     ['HEAD', 'GET', 'PUT', 'DELETE', 'OPTIONS', 'TRACE', 'POST'])),
            'pool_block': True
            }
        self._asession = FuturesSession(executor=executor)
        self._asession.mount('https://', HTTPAdapter(**adapter_kwargs))
        self._asession.mount('http://', HTTPAdapter(**adapter_kwargs))

        self._max_requests_workers = max_requests_workers
        self._token_manager = token_manager

    @property
    def executor(self):
        return self._asession.executor

    @property
    def first_completed(self):
        return cf.FIRST_COMPLETED

    @property
    def concurrent_futures(self):
        return cf

    @property
    def max_request_workers(self):
        return self._max_requests_workers

    def put(self, path, headers=None, callback=None, files=None, timeout=5.0):
        return self._send_request(
            params={
                'method': 'PUT',
                'url': '{base_url}/{path}'.format(base_url=self._base_url, path=path),
                'headers': headers or {},
                'files': files,
                'verify': (not self._disable_ssl_certificate),
                'timeout': timeout
                },
            on_finish_callback=callback)

    def _send_request(self, params, on_finish_callback):
        def extended_callback(session, response):
            if response.status == 401:
                self.logger.debug('Got a 401 error')
                self._renew_token()

            if on_finish_callback:
                on_finish_callback()

        c_params = params
        c_params['background_callback'] = extended_callback
        self.logger.debug('Making request to {0}'.format(params['url']))
        return self._asession.request(**c_params)
