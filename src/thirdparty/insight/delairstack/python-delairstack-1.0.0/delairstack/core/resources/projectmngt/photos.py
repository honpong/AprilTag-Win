from ..resources_manager_base import ResourcesManagerBase, Resource


class Photos(ResourcesManagerBase):
    _name = 'photos'

    def upload(self, photo: Resource, mission: Resource, files):
        pass

    def download(self, resource: Resource, destination_directory_path):
        pass
