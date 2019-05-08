import json
import logging

import urllib3

from ..errors import TokenRenewalError


class TokenManager(object):
    logger = logging.getLogger(__name__)

    def __init__(self, base_url, credentials, retry, disable_ssl_certificate):
        self._credentials = credentials
        self._retry = retry
        self._base_url = base_url

        self._credentials = {}
        if credentials is not None:
            self._credentials = credentials

        cert_reqs = 'CERT_NONE'

        if not disable_ssl_certificate:
            cert_reqs = 'CERT_REQUIRED'
        else:
            urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

        self._http = urllib3.PoolManager(cert_reqs=cert_reqs)

    def renew_token(self):
        self.logger.debug('Trying to get a new token...')
        headers = {
            'Authorization': 'Basic {secret_encode}'.format(secret_encode=self._credentials.secret_encode),
            'Content-Type': 'application/x-www-form-urlencoded'
            }
        response = self._http.request(
            method='POST',
            url='{base_url}/{credentials_url}'.format(base_url=self._base_url,
                                                      credentials_url=self._credentials.url),
            body=self._credentials.data,
            headers=headers,
            retries=self._retry)

        # decodes token response
        decoded_data = json.loads(response.data.decode('utf-8'))

        # update credentials
        if 'access_token' in decoded_data:
            self._credentials.access_token = decoded_data['access_token']
            self._credentials.token_type = decoded_data['token_type']
            self._credentials.expires_in = decoded_data['expires_in']
            self._credentials.refresh_token = decoded_data['refresh_token']
            self.logger.debug('Got a new token')
        else:
            self.logger.debug('No token has been retrieved')
            raise TokenRenewalError()

    @property
    def credentials(self):
        return self._credentials
