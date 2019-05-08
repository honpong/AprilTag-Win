"""Connection credentials.

"""

import base64
import urllib.parse

__all__ = ('ClientCredentials', 'UserCredentials')


class _Credentials(object):
    """Base class for credentials.

    """

    def __init__(self, data, secret_encode):
        self._token_type = ''
        self._expires_in = 0
        self._refresh_token = ''

        _auth_path_name = 'dxauth'
        _auth_get_path = 'oauth/token'
        self._url = '{auth_path}/{auth_get_path}'.format(auth_path=_auth_path_name,
                                                         auth_get_path=_auth_get_path)
        self._data = data
        self._secret_encode = secret_encode
        self._access_token = ''

    @property
    def secret_encode(self):
        return self._secret_encode

    @property
    def data(self):
        return self._data

    @property
    def url(self):
        return self._url

    @property
    def token_type(self):
        return self._token_type

    @property
    def refresh_token(self):
        return self._refresh_token

    @property
    def expires_in(self):
        return self._expires_in

    @property
    def access_token(self):
        return self._access_token

    @access_token.setter
    def access_token(self, value):
        self._access_token = value

    @refresh_token.setter
    def refresh_token(self, value):
        self._refresh_token = value

    @token_type.setter
    def token_type(self, value):
        self._token_type = value

    @expires_in.setter
    def expires_in(self, value):
        self._expires_in = value


class ClientCredentials(_Credentials):
    """API Client credentials.

    """

    def __init__(self, client_id, secret):
        super().__init__(data=urllib.parse.urlencode({'grant_type': 'client_credentials'}),
                         secret_encode=base64.b64encode((client_id + ':' + secret).encode()).decode())


class UserCredentials(_Credentials):
    """User credentials.

    """
    def __init__(self, user, password):
        secret = 'abc123:ssh-secret'
        super().__init__(data=urllib.parse.urlencode({
            'grant_type': 'password', 'username': user, 'password':
                password
            }),
            secret_encode=base64.b64encode(secret.encode()).decode())
