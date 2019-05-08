"""Files uploader.

"""

import functools
import json
import logging
import math
import os
import xml.etree.ElementTree as ET

from ..errors import UploadError, ResponseError


class Chunk(object):
    """Store the state of the upload of a file chunk.

    """

    def __init__(self, index, *, size, status='preupload', etag=None):
        self.index = index
        self.size = size
        self.status = status
        self.etag = etag
        self.attempt = 0
        self.req = None

    def __str__(self):
        return 'Chunk {index}: {size} {status} {attempt} {etag} {req}'.format(
            index=self.index, size=self.size, status=self.status,
            attempt=self.attempt,
            etag=self.etag if self.etag is not None else '<unknown>',
            req=self.req if self.req is not None else '<unknown>')


def prepare_chunks(*, file_size, chunk_size):
    """Prepare chunks for upload of a file of given size.

    It raises a ValueError when chunk_size is negative.

    """
    if chunk_size <= 0:
        raise ValueError('Expecting a positive chunk size')

    chunk_count = max(0, math.ceil(file_size / chunk_size))
    chunks = []
    if chunk_count > 0:
        for index in range(chunk_count):
            size = min(chunk_size, file_size - index * chunk_size)
            chunk = Chunk(index, size=size)
            chunks.append(chunk)
    return chunks


class MultipartUpload(object):
    """Send a given file in multiple requests.

    """
    logger = logging.getLogger(__name__)

    def __init__(self, connection, base_url, *, chunk_size=1024 * 1024 * 5):

        self._base_url = base_url
        self._chunk_size = max(1024 * 1024, chunk_size)
        self._connection = connection

        # temporary attributes set temporarily during execution of
        # send()
        self._src_path = None
        self._file_name = None
        self._file_name_no_ext = None
        self._file_ext = None
        self._oid = None
        self._upload_id = None
        self._chunks = None

    def is_configured(self):
        return self._file_name is not None and self._oid \
               and self._chunks is not None

    def is_started(self):
        return self.is_configured() and self._upload_id is not None

    @property
    def configure_url(self):
        if not self.is_configured():
            raise UploadError('Expecting a configured upload')

        url_tmpl = '{base_url}/dxobjects/{oid}/preuploadfiles'
        return url_tmpl.format(base_url=self._base_url,
                               oid=self._oid)

    @property
    def start_url(self):
        if not self.is_configured():
            raise UploadError('Expecting a configured upload')

        url_tmpl = '{base_url}/dxobjects/{oid}/{file_name}' \
                   '?uploads=[{oid}]'
        url = url_tmpl.format(base_url=self._base_url,
                              oid=self._oid,
                              file_name=self._file_name_no_ext)
        return url

    def chunk_url(self, chunk):
        if not self.is_started():
            raise UploadError('Expecting a started upload')

        url_tmpl = '{base_url}/dxobjects/{oid}/{file_name}' \
                   '?partNumber={index}&uploadId={upload_id}'
        url = url_tmpl.format(base_url=self._base_url,
                              oid=self._oid,
                              file_name=self._file_name_no_ext,
                              index=chunk.index,
                              upload_id=self._upload_id)
        return url

    @property
    def complete_url(self):
        if not self.is_started():
            raise UploadError('Expecting a started upload')

        url_tmpl = '{base_url}/dxobjects/{oid}/{file_name}' \
                   '?uploadId={upload_id}'
        url = url_tmpl.format(base_url=self._base_url,
                              oid=self._oid,
                              file_name=self._file_name_no_ext,
                              upload_id=self._upload_id)
        return url

    @property
    def finish_url(self):
        if not self.is_started():
            raise UploadError('Expecting a started upload')

        url_tmpl = '{base_url}/dxobjects/{oid}/postuploadfiles'
        url = url_tmpl.format(base_url=self._base_url,
                              oid=self._oid)
        return url

    @property
    def _ongoing_chunks(self):
        return [c for c in self._chunks
                if c.status == 'preupload' and c.req is not None
                and not c.req.done()]

    @property
    def _unfinished_chunks(self):
        return [c for c in self._chunks
                if c.status not in ('available', 'failed')]

    @property
    def _waiting_chunks(self):
        return [c for c in self._chunks
                if c.status not in ('available', 'failed') and c.req is None]

    def send(self, src_path, oid, *, name=None, ext=None):
        """Send the file at given path.

        Use the keywords arguments `name` and `ext` to specify the
        file name and extension. When one of those arguments is None,
        a default value is search from the `src_path`.

        It raises UploadError in case of failure.

        """
        if not os.path.exists(src_path):
            raise UploadError('File not found {}'.format(src_path))

        self._src_path = src_path
        src_file_name = os.path.basename(src_path)
        src_name, src_ext = os.path.splitext(src_file_name)
        src_ext = src_ext[1:] if len(src_ext) > 0 and src_ext[0] == '.' else ''
        self._file_name_no_ext = src_name if name is None else name
        self._file_ext = src_ext if ext is None else ext
        self._file_name = '{}.{}'.format(self._file_name_no_ext,
                                         self._file_ext)
        self._oid = oid
        self._upload_id = None
        self._chunks = None

        try:
            self.logger.debug("Configuring upload")
            self._configure()
            self.logger.debug("Starting upload")
            self._start()
            self.logger.debug("Completing upload")
            self._complete_upload()
            self.logger.debug("Finishing upload")
            self._finish()
        except UploadError as e:
            for n in ('_src_path', '_file_name', '_file_name_no_ext',
                      '_file_ext', '_oid', '_upload_id', '_chunks'):
                setattr(self, n, None)
            raise e

    def _configure(self):
        file_size = os.path.getsize(self._src_path)
        self._chunks = prepare_chunks(file_size=file_size,
                                      chunk_size=self._chunk_size)

        conf_desc = {
            'files': [{
                'header': {
                    'name': self._file_name,
                    'size': file_size,
                    'length': len(self._chunks)
                    }
                }]
            }

        headers = {
            'Cache-Control': 'no-cache',
            'Content-Type': 'application/json'
            }

        try:
            self._connection.post(
                path=self.configure_url,
                headers=headers,
                data=json.dumps(conf_desc))

        except ResponseError:
            raise UploadError('Upload configuration failed')

    def _start(self):
        try:
            content = self._connection.put(self.start_url)
        except ResponseError:
            raise UploadError('Failed to start upload')

        root = ET.fromstring(content)
        upload_elmt = root.find('UploadId')
        if upload_elmt is not None:
            self._upload_id = upload_elmt.text
        else:
            raise UploadError('Unexpected response to upload start')

        max_simultaneous = self._connection.asynchronous.max_request_workers

        def update_chunk(chunk, sess, resp):
            if resp.status_code == 200:
                chunk.status = 'available'
                chunk.etag = resp.headers['Etag']
            else:
                chunk.status = 'failed'
            chunk.req = None

        connection_delay = 30.0
        request_delay = 10.0
        with open(self._src_path, 'rb') as st:
            while len(self._unfinished_chunks) > 0:
                # limit the number of simultaneous enqueued requests
                queued_requests = self._connection.asynchronous.executor._work_queue.qsize()
                if queued_requests >= max_simultaneous:
                    reqs = [c.req for c in self._ongoing_chunks]
                    self._connection.asynchronous.concurrent_futures.wait(
                        reqs, timeout=request_delay,
                        return_when=self._connection.asynchronous.first_completed)
                    continue

                # check whether all chunks have been sent
                candidates = self._waiting_chunks
                if len(candidates) == 0:
                    break

                # send first candidate
                chunk = candidates[0]
                chunk.attempt += 1

                headers = {'Content-MD5': 'md5-hash'}

                offset = chunk.index * self._chunk_size
                st.seek(offset)
                blob = st.read(chunk.size)
                cb = functools.partial(update_chunk, chunk)
                chunk.req = self._connection.asynchronous.put(
                    path=self.chunk_url(chunk),
                    headers=headers,
                    files=(('part', blob),),
                    callback=cb,
                    timeout=connection_delay)

            reqs = [c.req for c in self._ongoing_chunks]
            self._connection.asynchronous.concurrent_futures.wait(
                reqs, timeout=request_delay,
                return_when=self._connection.asynchronous.concurrent_futures.ALL_COMPLETED)

        if any(map(lambda ch: ch.status == 'failed', self._chunks)):
            raise UploadError('Failed to upload some chunks')

    def _complete_upload(self):
        root = ET.Element('CompleteMultipartUpload')
        for chunk in self._chunks:
            part_elmt = ET.SubElement(root, 'Part')
            index_elmt = ET.SubElement(part_elmt, 'PartNumber')
            index_elmt.text = str(chunk.index)
            etag_elmt = ET.SubElement(part_elmt, 'ETag')
            etag_elmt.text = chunk.etag

        headers = {
            'Cache-Control': 'no-cache',
            'Content-Type': 'application/json'
            }

        xml = ET.tostring(root, encoding='unicode')
        try:
            self._connection.put(path=self.complete_url,
                                 headers=headers,
                                 data=json.dumps({'xml': xml}))
        except ResponseError:
            raise UploadError('Failed to complete upload')

    def _finish(self):
        headers = {
            'Cache-Control': 'no-cache',
            'Content-Type': 'application/json'
            }

        body = json.dumps({
            'filename': self._file_name_no_ext,
            'status': 'successfull'
            })
        try:
            self._connection.post(path=self.finish_url,
                                  headers=headers,
                                  data=body)
        except ResponseError:
            raise UploadError('Failed to finish upload')
