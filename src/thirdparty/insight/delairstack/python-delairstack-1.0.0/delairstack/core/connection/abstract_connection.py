from urllib3.util.retry import Retry

from .token import TokenManager


class AbstractConnection(object):

    def __init__(self, base_url=None, disable_ssl_certificate=False, credentials=None, token_manager=None,
                 max_retries=5):
        if base_url is None:
            raise ValueError('The URL has to be defined')

        if credentials is None and token_manager is None:
            raise ValueError('Credentials must be defined')

        self._disable_ssl_certificate = disable_ssl_certificate
        self._base_url = base_url
        self._max_retries = max_retries

        # defines retry policies
        # this one is used after getting a new token
        self._retry_with_auth = Retry(total=self._max_retries, backoff_factor=1,
                                      status_forcelist=[401, 403, 405, 409, 413, 429, 500, 502, 503, 504],
                                      method_whitelist=frozenset(
                                          ['HEAD', 'GET', 'PUT', 'DELETE', 'OPTIONS', 'TRACE', 'POST']))

        # this one is used for all requests, if a 401 error status code occurs, call renew_token with the other retry
        #  policy
        self._retry_without_auth = Retry(total=self._max_retries, backoff_factor=1,
                                         status_forcelist=[403, 405, 409, 413, 429, 500, 502, 503, 504],
                                         method_whitelist=frozenset(
                                             ['HEAD', 'GET', 'PUT', 'DELETE', 'OPTIONS', 'TRACE', 'POST']))

        if token_manager is None:
            self._token_manager = TokenManager(base_url=base_url,
                                               credentials=credentials,
                                               retry=self._retry_with_auth,
                                               disable_ssl_certificate=disable_ssl_certificate)
        else:
            self._token_manager = token_manager

    def _renew_token(self):
        self._token_manager.renew_token()

    def _add_authorization_maybe(self, headers):
        creds = self._token_manager.credentials
        if creds.access_token:
            auth = '{token_type} {access_token}'.format(token_type=creds.token_type,
                                                        access_token=creds.access_token)
            headers['Authorization'] = auth
