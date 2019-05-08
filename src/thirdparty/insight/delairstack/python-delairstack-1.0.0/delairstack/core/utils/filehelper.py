import os
import platform
import tempfile

"""
Helper class containing functions that assist in working with files.
"""

BLOCK_SIZE = 4096


class FileHelper(object):
    @staticmethod
    def write_as_file(file_path, response):
        """
        serialize the object to a file. The response comes from the Connection response allowing streaming response
        """
        with open(file_path, 'wb') as f:
            for chunk in response.stream(BLOCK_SIZE):
                if chunk:  # filter out keep-alive new chunks
                    f.write(chunk)

    @staticmethod
    def get_base_name_without_extension(file_path):
        if file_path == '' or file_path is None:
            return ''

        split = os.path.splitext(os.path.basename(file_path))
        if len(split) == 0:
            return ''

        return split[0]

    def get_local_temp_dir(self):
        return '/tmp' if platform.system() == 'Darwin' else tempfile.gettempdir()

    @staticmethod
    def get_file_extension(filename):
        split_fn = os.path.splitext(filename)

        if len(split_fn[1]) <= 1:
            raise Exception('Invalid Local File extension ' + filename)

        return split_fn[1][1:].lower()
